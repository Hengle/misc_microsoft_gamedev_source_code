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
// AkPlayingMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkPlayingMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkPBI.h"
#include "IAkTransportAware.h"
#include "AkActionPlayAndContinue.h"
#include "AkEvent.h"
#include "AkAudioMgr.h"
#include "AkAudioNode.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkURenderer.h"
#include "AkRanSeqBaseInfo.h"
#include "AkPositionRepository.h"

extern CAkAudioMgr*		g_pAudioMgr;
extern AkMemPoolId		g_DefaultPoolId;

#define AK_PLAYING_MGR_MIN_NUM_PER_PLAYINGID_INSTANCES 8

AKRESULT CAkPlayingMgr::Init()
{
	return m_PlayingMap.Init( g_DefaultPoolId );
}

void CAkPlayingMgr::Term()
{
#ifndef AK_OPTIMIZED
	for( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
	{
		(*iter).item.m_PBIList.Term();
	}

#endif

	m_PlayingMap.Term();
}

AKRESULT CAkPlayingMgr::AddPlayingID( AkQueuedMsg_EventBase & in_event,
									 AkCallbackFunc in_pfnCallback,
									 void*			in_pCookie,
									 AkUInt32		in_uiRegisteredNotif,
									 AkUniqueID		in_id )
{
	AkAutoLock<CAkLock> lock( m_csLock );

	PlayingMgrItem* pItem = m_PlayingMap.Set( in_event.PlayingID );
	if ( !pItem )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	if ( pItem->m_PBIList.Init( AK_PLAYING_MGR_MIN_NUM_PER_PLAYINGID_INSTANCES, AK_NO_MAX_LIST_SIZE,g_DefaultPoolId ) != AK_Success ) 
	{
		m_PlayingMap.Unset( in_event.PlayingID );
		return AK_Fail;
	}
#else
	pItem->cPBI = 0;
#endif

	pItem->cAction = 1;
	pItem->uniqueID = in_id;
	pItem->GameObj = in_event.GameObjID;
	pItem->CustomParam = in_event.CustomParam;
	pItem->pfnCallback = in_pfnCallback;
	pItem->pCookie = in_pCookie;
	if ( !in_pfnCallback )
		in_uiRegisteredNotif &= ~( AK_CallbackBits ); // remove callback bits if no callback
	pItem->uiRegisteredNotif = in_uiRegisteredNotif;

	return AK_Success;
}

void CAkPlayingMgr::CancelCallbackCookie( void* in_pCookie )
{
	AkAutoLock<CAkLock> localLock( m_csLock );

	for ( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
	{
		if( (*iter).item.pCookie == in_pCookie )
		{
			(*iter).item.pfnCallback = NULL;
			(*iter).item.uiRegisteredNotif &= ~( AK_CallbackBits ); // remove callback bits if no callback
		}
	}
}

void CAkPlayingMgr::CancelCallback( AkPlayingID in_playingID )
{
	AkAutoLock<CAkLock> localLock( m_csLock );
	for ( AkPlayingMap::Iterator iter = m_PlayingMap.Begin(); iter != m_PlayingMap.End(); ++iter )
	{
		if( (*iter).key == in_playingID )
		{
			(*iter).item.pfnCallback = NULL;
			(*iter).item.uiRegisteredNotif &= ~( AK_CallbackBits ); // remove callback bits if no callback
			break;
		}
	}
}

void CAkPlayingMgr::CheckRemovePlayingID( AkPlayingID in_PlayingID, PlayingMgrItem* in_pItem )
{
#ifndef AK_OPTIMIZED
	if( in_pItem->m_PBIList.IsEmpty() && in_pItem->cAction == 0 )
	{
		in_pItem->m_PBIList.Term();
#else
	if( in_pItem->cPBI == 0 && in_pItem->cAction == 0 )
	{
#endif
		if( in_pItem->uiRegisteredNotif & AK_EndOfEvent )
		{
			AKASSERT( in_pItem->pfnCallback );
			AkEventCallbackInfo info;

			info.pCookie = in_pItem->pCookie;
			info.gameObjID = in_pItem->GameObj;
			info.playingID = in_PlayingID;
			info.eventID = in_pItem->uniqueID;

			(*in_pItem->pfnCallback)( AK_EndOfEvent, &info );
		}

		if( in_pItem->uiRegisteredNotif & AK_EnableGetSourcePlayPosition )
		{
			g_pPositionRepository->RemovePlayingID( in_PlayingID );
		}

		MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_pItem->GameObj, in_pItem->CustomParam, in_pItem->uniqueID );

		m_PlayingMap.Unset( in_PlayingID );
	}
}

void CAkPlayingMgr::GetNotificationInformation( CAkPBI* in_pPBI,
											    bool* out_pWantMarkerInformation,
											    bool* out_pWantPositionInformation
											   )
{
	AKASSERT( out_pWantMarkerInformation != NULL );
	AKASSERT( out_pWantPositionInformation != NULL );
	AkPlayingID l_playingID = in_pPBI->GetPlayingID();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( l_playingID );
	AKASSERT( pItem );

	*out_pWantMarkerInformation = false;
	*out_pWantPositionInformation = false;
	if( pItem )
	{
		if( pItem->uiRegisteredNotif & AK_Marker )
		{
			AKASSERT( pItem->pfnCallback );
			*out_pWantMarkerInformation = true;
		}

		if( pItem->uiRegisteredNotif & AK_EnableGetSourcePlayPosition )
		{
			*out_pWantPositionInformation = true;
		}
	}
}

void CAkPlayingMgr::NotifyEndOfDynamicSequenceItem(
	CAkPBI* in_pPBI,
	AkUniqueID in_audioNodeID,
	void* in_pCustomInfo
	)
{
	AkPlayingID playingID = in_pPBI->GetPlayingID();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( playingID );
	if ( pItem && pItem->uiRegisteredNotif & AK_EndOfDynamicSequenceItem )
	{
		AKASSERT( pItem->pfnCallback );
		AkDynamicSequenceItemCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;

		info.playingID = playingID;
		info.audioNodeID = in_audioNodeID;
		info.pCustomInfo = in_pCustomInfo;

		(*pItem->pfnCallback)( AK_EndOfDynamicSequenceItem, &info );
	}
}

void CAkPlayingMgr::NotifyMarker( CAkPBI* in_pPBI, AkAudioMarker* in_pMarker )
{
	AkAutoLock<CAkLock> localLock( m_csLock );

	AkPlayingID l_playingID = in_pPBI->GetPlayingID();
	PlayingMgrItem* pItem = m_PlayingMap.Exists( l_playingID );
	AKASSERT( pItem );

	if( pItem->uiRegisteredNotif & AK_Marker )
	{
		AKASSERT( pItem->pfnCallback );
		AkMarkerCallbackInfo info;

		info.pCookie = pItem->pCookie;
		info.gameObjID = pItem->GameObj;
		info.playingID = l_playingID;
		info.eventID = pItem->uniqueID;

		info.uIdentifier = in_pMarker->dwIdentifier;
		info.uPosition = in_pMarker->dwPosition;
		info.strLabel = in_pMarker->strLabel;
			
		(*pItem->pfnCallback)( AK_Marker, &info );
	}

	MONITOR_EVENTMARKERNOTIF( l_playingID, pItem->GameObj, pItem->CustomParam, pItem->uniqueID, in_pMarker->strLabel );
}

void CAkPlayingMgr::NotifyMarkers( AkBufferMarker*& io_pCurrMarker, AkUInt16 & in_uNumMarkers )
{
	// Notify the markers if needed
	if( io_pCurrMarker )
	{
		AkBufferMarker* pCurrMarker = io_pCurrMarker;
		for( AkUInt32 iCurrMarker = 0; iCurrMarker < in_uNumMarkers; iCurrMarker++ )
		{
			NotifyMarker( pCurrMarker->pContext, &pCurrMarker->marker );

			pCurrMarker++;
		}

		// free the markers
		AkFree( AK_MARKERS_POOL_ID, io_pCurrMarker );
		io_pCurrMarker = NULL;
		in_uNumMarkers = 0;
	}
}

AKRESULT CAkPlayingMgr::SetPBI( AkPlayingID in_PlayingID, IAkTransportAware* in_pPBI )
{
	AkAutoLock<CAkLock> lock( m_csLock );

	AKRESULT eResult = AK_Success;

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );

#ifndef AK_OPTIMIZED
	if ( !pItem || !pItem->m_PBIList.AddLast( in_pPBI ) ) 
		eResult = AK_Fail;
#else
	if ( pItem )
		pItem->cPBI++;
#endif

	return eResult;
}

void CAkPlayingMgr::Remove(AkPlayingID in_PlayingID, IAkTransportAware* in_pPBI)
{
	AkAutoLock<CAkLock> lock( m_csLock );

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if( pItem )
	{
#ifndef AK_OPTIMIZED
		pItem->m_PBIList.Remove(in_pPBI);
#else
		pItem->cPBI--;
#endif
		CheckRemovePlayingID( in_PlayingID, pItem );
	}
}

bool CAkPlayingMgr::IsActive(AkPlayingID in_PlayingID)
{
	AkAutoLock<CAkLock> lock( m_csLock );

	return ( m_PlayingMap.Exists(in_PlayingID) != NULL);
}

void CAkPlayingMgr::AddItemActiveCount(AkPlayingID in_PlayingID)
{
	if( in_PlayingID )
	{
		AkAutoLock<CAkLock> lock( m_csLock );

		PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );

		if ( pItem )
			++pItem->cAction;
	}
}

void CAkPlayingMgr::RemoveItemActiveCount(AkPlayingID in_PlayingID)
{
	AkAutoLock<CAkLock> lock( m_csLock );

	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	// We must check since the action may not have been given a valid Playing ID, 
	// It is not an error, it will happen for example when a Duck Action is kicked from the lisk ion terminaison
	if( pItem )
	{
		--( pItem->cAction );
		AKASSERT( pItem->cAction >= 0 );
		CheckRemovePlayingID( in_PlayingID, pItem );
	}
}

#ifndef AK_OPTIMIZED

void CAkPlayingMgr::StopAndContinue(
		AkPlayingID				in_PlayingID,
		CAkRegisteredObj *		in_GameObjPtr,
		CAkContinueListItem&	in_ContinueListItem,
		AkUniqueID				in_ItemToPlay,
		AkUInt16				in_wPosition,
        CAkPBIAware*            in_pInstigator
		)
{
	AkAutoLock<CAkLock> lock( m_csLock );

	AKASSERT(g_pAudioMgr);
	PlayingMgrItem* pItem = m_PlayingMap.Exists( in_PlayingID );
	if( pItem )
	{
		pItem->cAction++;
		AkPendingAction* pPendingAction =  g_pAudioMgr->GetActionMatchingPlayingID(in_PlayingID);
		CAkActionPlayAndContinue* pActionPAC = NULL;
		if( pPendingAction )
		{
			AKASSERT( pPendingAction->pAction->ActionType() == AkActionType_PlayAndContinue );
			pActionPAC = static_cast<CAkActionPlayAndContinue*>(pPendingAction->pAction);
			if( pActionPAC->GetContinuationList() )
			{
				if( in_ContinueListItem.m_LoopingInfo.bIsEnabled )
				{
					CAkContinueListItem & itemFirst = *(pActionPAC->GetContinuationList()->m_listItems.Begin());

					in_ContinueListItem.m_LoopingInfo.lLoopCount = itemFirst.m_LoopingInfo.lLoopCount;
					if( itemFirst.m_pContainerInfo )
					{
						CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>( itemFirst.m_pContainerInfo );
						if( pSeqInfo->m_i16LastPositionChosen == 0 && pSeqInfo->m_bIsForward )
						{
							in_ContinueListItem.m_LoopingInfo.lLoopCount += 1;
						}
					}
				}
			}
		}
		if( !( pItem->m_PBIList.IsEmpty() ) )
		{
			g_pAudioMgr->ClearPendingItems( in_PlayingID );
			
			size_t i = 0;
			for( PlayingMgrItem::AkPBIList::Iterator iter = pItem->m_PBIList.Begin(); iter != pItem->m_PBIList.End(); ++iter )
			{
				if( i < pItem->m_PBIList.Length() - 1 )
				{
					(*iter)->_Stop( AkPBIStopMode_StopAndContinueSequel, true );
				}
				else
				{
					(*iter)->_StopAndContinue( in_ItemToPlay, in_wPosition, &in_ContinueListItem );
				}
				++i;
			}
		}
		else// Must take info from a pending one
		{
			if(pPendingAction)
			{
				// create the action we need
				CAkSmartPtr<CAkContinuationList> l_spContList;
				l_spContList.Attach( CAkContinuationList::Create() );
				if( l_spContList )
				{
					AKRESULT eResult = l_spContList->m_listItems.AddLast( in_ContinueListItem ) ? AK_Success : AK_Fail;

					if( eResult == AK_Success )
					{
						CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, l_spContList );
						if( pAction )
						{
							PlayHistory History;
							History.HistArray.Init();
							History.Add( in_wPosition, true );
							pAction->SetHistory(History);
							pAction->SetElementID(in_ItemToPlay);
                            pAction->SetInstigator( in_pInstigator );

							AkPendingAction* pPendingAction2 = AkNew( g_DefaultPoolId, AkPendingAction( in_GameObjPtr ) );
							if( pPendingAction2 )
							{
								(*l_spContList->m_listItems.Begin()).m_LoopingInfo = (*pActionPAC->GetContinuationList()->m_listItems.Begin()).m_LoopingInfo;

								// copy the transitions we had in the pending action
								bool l_bIsFading;
								CAkTransition* l_pTransition = pActionPAC->GetPlayStopTransition( l_bIsFading );
								pAction->SetPlayStopTransition( l_pTransition, l_bIsFading, pPendingAction2 );
								l_pTransition = pActionPAC->GetPauseResumeTransition( l_bIsFading );
								pAction->SetPauseResumeTransition( l_pTransition, l_bIsFading, pPendingAction2 );

								pAction->SetPathInfo(pActionPAC->GetPathInfo());

								pPendingAction2->pAction = pAction;
								pPendingAction2->UserParam.CustomParam = pPendingAction->UserParam.CustomParam;
								pPendingAction2->UserParam.PlayingID = in_PlayingID;

								g_pAudioMgr->ClearPendingItemsExemptOne(in_PlayingID);

								g_pAudioMgr->EnqueueOrExecuteAction(pPendingAction2);
							}

							// We are done with these
							pAction->Release();
						}
					}
				}
			}
		}
		pItem->cAction--;
	}
}

#endif
