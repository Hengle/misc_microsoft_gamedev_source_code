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
// AkStateMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkStateMgr.h"
#include "AkActionSetState.h"
#include "AkAudioLib.h"
#include "AkAudioLibIndex.h"
#include "AkEvent.h"
#include "AkParameterNode.h"
#include "AkTransitionManager.h"
#include "AkMonitor.h"
#include "AkSwitchCntr.h"
#include "AudiolibDefs.h"

#define AK_MIN_MEMBER_PER_STATE_GROUP 12
#define AK_MAX_MEMBER_PER_STATE_GROUP AK_NO_MAX_LIST_SIZE

#define AK_MIN_NUM_TRANSITION_TIME 4
#define AK_MAX_NUM_TRANSITION_TIME AK_NO_MAX_LIST_SIZE

#define AK_REGISTERED_LIST_SIZE		8

extern AkMemPoolId g_DefaultPoolId;

extern AkExternalStateHandlerCallback g_pExternalStateHandler;

AKRESULT AkStateGroupInfo::Init()
{
	AKRESULT eResult = listMemberNodes.Init( AK_MIN_MEMBER_PER_STATE_GROUP, AK_MAX_MEMBER_PER_STATE_GROUP,g_DefaultPoolId );

	return eResult;
}

void AkStateGroupInfo::Term()
{
	mapStates.Term();
	mapTransitions.Term();
	listMemberNodes.Term();
}

AKRESULT CAkStateMgr::PreparationStateItem::Notify( AkUInt32 in_GameSyncID, bool in_bIsActive )
{
	AKRESULT eResult = AK_Success;
	for( PreparationList::Iterator iter = m_PreparationList.Begin(); iter != m_PreparationList.End(); ++iter )
	{
		eResult = iter.pItem->ModifyActiveState( in_GameSyncID, in_bIsActive );
		if( eResult != AK_Success )
		{
			if( in_bIsActive )// do not remove the if, even if there is an assert after.
			{
				// Must cancel what has been done up to now.
				for( PreparationList::Iterator iterFlush = m_PreparationList.Begin(); iter != iterFlush; ++iterFlush )
				{
					iterFlush.pItem->ModifyActiveState( in_GameSyncID, false );
				}
			}
			else
			{
				AKASSERT( !"Automatic data preparation system :: Unhandled situation" );
			}
			break;
		}
	}
	return eResult;
}

CAkStateMgr::CAkStateMgr()
{
}

CAkStateMgr::~CAkStateMgr()
{
}

AKRESULT CAkStateMgr::Init()
{
	AKRESULT eResult = AK_Success;

	eResult = m_listRegisteredSwitch.Init( AK_REGISTERED_LIST_SIZE, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
	if( eResult == AK_Success )
	{
		eResult = m_listRegisteredTrigger.Init( AK_REGISTERED_LIST_SIZE, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
	}
	m_PreparationGroupsStates.Init();
	m_PreparationGroupsSwitches.Init();

	return eResult;
}

void CAkStateMgr::Term()
{
	RemoveAllStates();

	RemoveAllStateGroups( false );

	m_StateGroups.Term();
	m_listRegisteredSwitch.Term();
	m_listRegisteredTrigger.Term();

	TermPreparationGroup( m_PreparationGroupsStates );
	TermPreparationGroup( m_PreparationGroupsSwitches );
}

void CAkStateMgr::TermPreparationGroup( PreparationGroups& in_rGroup )
{
	PreparationGroups::IteratorEx iter = in_rGroup.BeginEx();
	while( iter != in_rGroup.End() )
	{
		PreparationStateItem* pItem = iter.pItem;
		iter = in_rGroup.Erase( iter );
		AkDelete2( g_DefaultPoolId, PreparationStateItem , pItem );
	}
	in_rGroup.Term();
}

AKRESULT CAkStateMgr::AddStateGroup(AkStateGroupID in_ulStateGroupID)
{
	AKRESULT eResult = AK_Success;

	AKASSERT(in_ulStateGroupID);//zero is an invalid ID

	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists(in_ulStateGroupID);
	if( l_ppStateGrpInfo )
	{
		return AK_Success;
	}
	AkStateGroupInfo* l_pStateGroupInfo = AkNew( g_DefaultPoolId, AkStateGroupInfo );
	if( !l_pStateGroupInfo )
	{
		return AK_Fail;
	}
	eResult = l_pStateGroupInfo->Init();
	if( eResult == AK_Success )
	{
		if( !m_StateGroups.Set( in_ulStateGroupID, l_pStateGroupInfo ) )
		{
			eResult = AK_Fail;
		}
	}
	if( eResult != AK_Success )
	{
		l_pStateGroupInfo->Term();
		AkDelete( g_DefaultPoolId, l_pStateGroupInfo );
	}

	return eResult;
}

AKRESULT CAkStateMgr::AddStateGroupMember(AkStateGroupID in_ulStateGroupID, CAkParameterNodeBase* in_pMember)
{
	AKASSERT(in_pMember);
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists(in_ulStateGroupID);
	if( l_ppStateGrpInfo )
	{
		if( (*l_ppStateGrpInfo)->listMemberNodes.AddLast( in_pMember ) )
			return AK_Success;
		else
			return AK_InsufficientMemory;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

AKRESULT CAkStateMgr::SetdefaultTransitionTime(AkStateGroupID in_ulStateGroupID,AkTimeMs lTransitionTime)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists(in_ulStateGroupID);
	if( l_ppStateGrpInfo )
	{
		(*l_ppStateGrpInfo)->lDefaultTransitionTime = lTransitionTime;
		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

#ifndef AK_OPTIMIZED
AKRESULT CAkStateMgr::RemoveStateGroup(AkStateGroupID in_ulStateGroupID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists(in_ulStateGroupID);
	if( l_ppStateGrpInfo )
	{
		RemoveStates(in_ulStateGroupID);
        AkStateGroupInfo* pStateGroupInfo = *l_ppStateGrpInfo;

        // Set the actual state of this channel's member to None.
		while( !pStateGroupInfo->listMemberNodes.IsEmpty() )
	    {
			CAkParameterNodeBase* pNode = pStateGroupInfo->listMemberNodes.First();
			pNode->SetActualState(0);
			pNode->SetStateGroup(0); // SetStateGroup(0) the member is removed from pStateGroupInfo->setMemberNodes	
	    }

		pStateGroupInfo->Term();
		AkDelete( g_DefaultPoolId, pStateGroupInfo );
		m_StateGroups.Unset( in_ulStateGroupID );

		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}
#endif

AKRESULT CAkStateMgr::RemoveStateGroupMember(AkStateGroupID in_ulStateGroupID, CAkParameterNodeBase* in_pMember)
{
	AKASSERT(in_pMember);

	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if( l_ppStateGrpInfo )
	{
		(*l_ppStateGrpInfo)->listMemberNodes.Remove( in_pMember );
		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

AKRESULT CAkStateMgr::AddStateTransition(AkStateGroupID in_ulStateGroupID, AkStateID in_ulStateID1, AkStateID in_ulStateID2, AkTimeMs lTransitionTime,bool in_bIsShared/*= false*/)
{
	AKASSERT(in_ulStateGroupID);
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if( l_ppStateGrpInfo )
	{
		AkStateTransition localTransition;
		localTransition.StateFrom = in_ulStateID1;
		localTransition.StateTo = in_ulStateID2;
		if( !(*l_ppStateGrpInfo)->mapTransitions.Set( localTransition, lTransitionTime ) )
		{
			return AK_Fail;
		}
		if(in_bIsShared)
		{
			localTransition.StateFrom = in_ulStateID2;
			localTransition.StateTo = in_ulStateID1;
			if( !(*l_ppStateGrpInfo)->mapTransitions.Set( localTransition, lTransitionTime ) )
			{
				return AK_Fail;
			}
		}
		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

AKRESULT CAkStateMgr::RemoveStateTransition(AkStateGroupID in_ulStateGroupID, AkStateID in_ulStateID1, AkStateID in_ulStateID2, bool in_bIsShared/*= false*/)
{
	AKASSERT(in_ulStateGroupID);

	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if( l_ppStateGrpInfo )
	{
		AkStateTransition localTransition;
		localTransition.StateFrom = in_ulStateID1;
		localTransition.StateTo = in_ulStateID2;
		(*l_ppStateGrpInfo)->mapTransitions.Unset( localTransition );
		if(in_bIsShared)
		{
			localTransition.StateFrom = in_ulStateID2;
			localTransition.StateTo = in_ulStateID1;
			(*l_ppStateGrpInfo)->mapTransitions.Unset( localTransition );
		}
		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

AKRESULT CAkStateMgr::ClearStateTransition(AkStateGroupID in_ulStateGroupID)
{
	AKASSERT(in_ulStateGroupID);
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if( l_ppStateGrpInfo )
	{
		(*l_ppStateGrpInfo)->mapTransitions.RemoveAll();
		return AK_Success;
	}
	else
	{
		return AK_InvalidStateGroup;
	}
}

void CAkStateMgr::SetStateInternal(AkStateGroupID in_ulStateGroupID, AkStateID in_ulStateID, bool in_bSkipTransitionTime /*= false */, bool in_bSkipExtension /*= false */ )
{
	AkStateGroupInfo*		pStateGroupInfo = NULL;
	
	AKASSERT(g_pTransitionManager);
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if( !l_ppStateGrpInfo )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidGroupID);
		return;
	}

	pStateGroupInfo = *l_ppStateGrpInfo;
	AKASSERT(pStateGroupInfo);
	
	if ( !in_bSkipExtension )
		UpdateSwitches( in_ulStateGroupID, pStateGroupInfo->ActualState, in_ulStateID );

	// in_bSkipTransitionTime means "force state change Now". Thus we don't call the extension handler.
    if ( !in_bSkipExtension && g_pExternalStateHandler && !in_bSkipTransitionTime )
    {
        if ( g_pExternalStateHandler( in_ulStateGroupID, in_ulStateID ) )
        {
            // External State Handler returned true, therefore it processed the state change. Leave now.
            return;
        }
    }
    
//----------------------------------------------------------------------------------------------------
// initialise transition parameters
// target type is assumed to be AkReal32 for all
// fade curve is assumed to be linear for all
//----------------------------------------------------------------------------------------------------

    TransitionParameters	VolumeParams, PitchParams, LfeParams, LPFParams;
	bool					NeedToNotify = false;

	VolumeParams.eTargetType = AkVolumeFloat;
	VolumeParams.eFadeCurve = AkCurveInterpolation_Linear;
	VolumeParams.bdBs = true;

	PitchParams.eTargetType = AkPitchLong;
	PitchParams.eFadeCurve = AkCurveInterpolation_Linear;
	PitchParams.bdBs = false;

	LPFParams.eTargetType = AkLPFFloat;
	LPFParams.eFadeCurve = AkCurveInterpolation_Linear;
	LPFParams.bdBs = false;

	LfeParams.eTargetType = AkLfeFloat;
	LfeParams.eFadeCurve = AkCurveInterpolation_Linear;
	LfeParams.bdBs = true;

	// assume we have none
	VolumeParams.uStartValue.fValue = 0.0f;
	PitchParams.uStartValue.lValue = 0;
	LPFParams.uStartValue.fValue = 0;
	LfeParams.uStartValue.fValue = 0.0f;

	// assume we have none
	VolumeParams.uTargetValue.fValue = 0.0f;
	PitchParams.uTargetValue.lValue = 0;
	LPFParams.uTargetValue.fValue = 0;
	LfeParams.uTargetValue.fValue = 0.0f;

//----------------------------------------------------------------------------------------------------
// now let's see if we can grab some information
//----------------------------------------------------------------------------------------------------
	
	// let's see if we have anything defined for this transition
	AkStateTransition ThisTransition;
	ThisTransition.StateFrom = pStateGroupInfo->ActualState;
	ThisTransition.StateTo = in_ulStateID;

	

	AkTimeMs Time = 0;
	if(!in_bSkipTransitionTime)
	{
		AkTimeMs* l_pTime = pStateGroupInfo->mapTransitions.Exists(ThisTransition);
		if( l_pTime )
		{
			Time = *l_pTime;
		}
		else
		{
			Time = pStateGroupInfo->lDefaultTransitionTime;
		}
	}

	VolumeParams.lDuration = Time;
	PitchParams.lDuration = Time;
	LPFParams.lDuration = Time;
	LfeParams.lDuration = Time;

	AkStateLink* l_pStateLink = pStateGroupInfo->mapStates.Exists(in_ulStateID);
	if( l_pStateLink )
	{
		pStateGroupInfo->StatePtr = l_pStateLink->pState;
	}
	else
	{
		pStateGroupInfo->StatePtr = NULL;
	}

//----------------------------------------------------------------------------------------------------
// go through the members list and start transitions when needed
//----------------------------------------------------------------------------------------------------
	for( AkStateGroupInfo::AkListMemberNode::Iterator iter = pStateGroupInfo->listMemberNodes.Begin(); iter != pStateGroupInfo->listMemberNodes.End(); ++iter )
	{
		CAkState*			pPreviousState	= NULL;
		CAkState*			pNextState		= NULL;
		CAkParameterNodeBase*	pParamNodeBase = NULL;

		pParamNodeBase = *iter;

		// figure out what the state we start from is
		pPreviousState = pParamNodeBase->GetState(ThisTransition.StateFrom);

		// got one ? then use it
		if(pPreviousState)
		{
			VolumeParams.uStartValue.fValue = pPreviousState->Volume();
			PitchParams.uStartValue.lValue = pPreviousState->Pitch();
			LPFParams.uStartValue.fValue = pPreviousState->LPF();
			LfeParams.uStartValue.fValue = pPreviousState->LFEVolume();
		}

		// set new state
		pParamNodeBase->SetActualState(in_ulStateID);

		// figure out what the state we are going to is
		pNextState = pParamNodeBase->GetState();

		// got one ? then use it
		if(pNextState)
		{
			VolumeParams.uTargetValue.fValue = pNextState->Volume();
			PitchParams.uTargetValue.lValue = pNextState->Pitch();
			LPFParams.uTargetValue.fValue = pNextState->LPF();
			LfeParams.uTargetValue.fValue = pNextState->LFEVolume();
		}

//----------------------------------------------------------------------------------------------------
// volume transition
//----------------------------------------------------------------------------------------------------
		if((VolumeParams.uStartValue.fValue != VolumeParams.uTargetValue.fValue) || (pParamNodeBase->GetTransition(TransitionTarget_Volume) != NULL) )
		{
			VolumeParams.pUser = pParamNodeBase;

			// have we got one running already ?
			CAkTransition * pTransition = pParamNodeBase->GetTransition(TransitionTarget_Volume);
			if( pTransition != NULL)
			{
				// yup, let's change the direction it goes
				g_pTransitionManager->ChangeParameter(pTransition,
														TransTarget_Volume,
														VolumeParams.uTargetValue,
														VolumeParams.lDuration,
														AkValueMeaning_Default);
			}
			else
			{
				pParamNodeBase->SetTransVolume(VolumeParams.uStartValue.fValue);
				NeedToNotify = true;
			}

			// let's start one if needed
			if(pTransition == NULL)
			{
				// PhM : AddTransitionToList() will return NULL if none is available
				pParamNodeBase->SetTransition(g_pTransitionManager->AddTransitionToList(VolumeParams, true, CAkTransitionManager::TC_State ),TransitionTarget_Volume);
			}
		}
		// no transition for this one
		else
		{
			pParamNodeBase->SetTransVolume(VolumeParams.uTargetValue.fValue);
			NeedToNotify = true;
		}
//----------------------------------------------------------------------------------------------------
// lfe transition
//----------------------------------------------------------------------------------------------------
		if((LfeParams.uStartValue.fValue != LfeParams.uTargetValue.fValue) || (pParamNodeBase->GetTransition(TransitionTarget_Lfe) != NULL) )
		{
			LfeParams.pUser = pParamNodeBase;

			// have we got one running already ?
			CAkTransition * pTransition = pParamNodeBase->GetTransition(TransitionTarget_Lfe);
			if( pTransition != NULL)
			{
				// yup, let's change the direction it goes
				g_pTransitionManager->ChangeParameter(pTransition,
														TransTarget_Lfe,
														LfeParams.uTargetValue,
														LfeParams.lDuration,
														AkValueMeaning_Default);
			}
			else
			{
				pParamNodeBase->SetTransLfe(LfeParams.uStartValue.fValue);
				NeedToNotify = true;
			}

			// let's start one if needed
			if(pTransition == NULL)
			{
				// PhM : AddTransitionToList() will return NULL if none is available
				pParamNodeBase->SetTransition(g_pTransitionManager->AddTransitionToList(LfeParams, true, CAkTransitionManager::TC_State ), TransitionTarget_Lfe);
			}
		}
		// no transition for this one
		else
		{
			pParamNodeBase->SetTransLfe(LfeParams.uTargetValue.fValue);
			NeedToNotify = true;
		}
//----------------------------------------------------------------------------------------------------
// pitch transition
//----------------------------------------------------------------------------------------------------
		if((PitchParams.uStartValue.lValue != PitchParams.uTargetValue.lValue) || (pParamNodeBase->GetTransition(TransitionTarget_Pitch) != NULL))
		{
			PitchParams.pUser = pParamNodeBase;

			// have we got one running already ?
			CAkTransition * pTransition = pParamNodeBase->GetTransition(TransitionTarget_Pitch);
			if( pTransition != NULL)
			{
				// yup, let's change the direction it goes
				g_pTransitionManager->ChangeParameter(pTransition,
														TransTarget_Pitch,
														PitchParams.uTargetValue,
														PitchParams.lDuration,
														AkValueMeaning_Default);
			}
			else
			{
				pParamNodeBase->SetTransPitch(static_cast<AkPitchValue>(PitchParams.uStartValue.lValue));
				NeedToNotify = true;
			}

			// let's start one if needed
			if(pTransition == NULL)
			{
				// PhM : AddTransitionToList() will return NULL if none is available
				pParamNodeBase->SetTransition(g_pTransitionManager->AddTransitionToList(PitchParams, true, CAkTransitionManager::TC_State ), TransitionTarget_Pitch);
			}
		}
		// no transition for this one
		else
		{
			pParamNodeBase->SetTransPitch(static_cast<AkPitchValue>(PitchParams.uTargetValue.lValue));
			NeedToNotify = true;
		}

//----------------------------------------------------------------------------------------------------
// LPF transition
//----------------------------------------------------------------------------------------------------
		if((LPFParams.uStartValue.fValue != LPFParams.uTargetValue.fValue) || (pParamNodeBase->GetTransition(TransitionTarget_LPF) != NULL))
		{
			LPFParams.pUser = pParamNodeBase;

			// have we got one running already ?
			CAkTransition * pTransition = pParamNodeBase->GetTransition(TransitionTarget_LPF);
			if( pTransition != NULL)
			{
				// yup, let's change the direction it goes
				g_pTransitionManager->ChangeParameter(pTransition,
														TransTarget_LPF,
														LPFParams.uTargetValue,
														LPFParams.lDuration,
														AkValueMeaning_Default);
			}
			else
			{
				pParamNodeBase->SetTransLPF(static_cast<AkLPFType>(LPFParams.uStartValue.fValue));
				NeedToNotify = true;
			}

			// let's start one if needed
			if(pTransition == NULL)
			{
				// PhM : AddTransitionToList() will return NULL if none is available
				pParamNodeBase->SetTransition(g_pTransitionManager->AddTransitionToList(LPFParams, true, CAkTransitionManager::TC_State ), TransitionTarget_LPF);
			}
		}
		// no transition for this one
		else
		{
			pParamNodeBase->SetTransLPF(static_cast<AkLPFType>(LPFParams.uTargetValue.fValue));
			NeedToNotify = true;
		}

		if(NeedToNotify)
		{
			pParamNodeBase->NotifyStateModified();
		}
	}

    pStateGroupInfo->ActualState = in_ulStateID;

	if ( ThisTransition.StateFrom != ThisTransition.StateTo )
	{
		MONITOR_STATECHANGED(in_ulStateGroupID, ThisTransition.StateFrom, ThisTransition.StateTo);
	}
}

AkStateID CAkStateMgr::GetState(AkStateGroupID in_ulStateGroupID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	if(l_ppStateGrpInfo)
		return (*l_ppStateGrpInfo)->ActualState;
	else
		return 0; // returning state none.
}

CAkState* CAkStateMgr::GetStatePtr(AkStateGroupID in_ulStateGroupID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	AKASSERT( l_ppStateGrpInfo );
	return (*l_ppStateGrpInfo)->StatePtr;
}

CAkState* CAkStateMgr::GetStatePtr(AkStateGroupID in_ulStateGroupID, AkStateID in_StateTypeID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	AKASSERT( l_ppStateGrpInfo );
	AkStateGroupInfo* pStateGroupInfo = *l_ppStateGrpInfo;
	
	AkStateLink* l_pStateLink = pStateGroupInfo->mapStates.Exists( in_StateTypeID );
	if( l_pStateLink )
	{
		return l_pStateLink->pState;
	}
	return NULL;
}

AKRESULT CAkStateMgr::AddState(AkStateGroupID in_ulStateGroupID,AkStateID in_ulStateID,AkUniqueID in_ulStateUniqueID)
{
	AKASSERT(g_pIndex);
	if(!in_ulStateID)
	{
		return AK_InvalidID;
	}
	CAkState* pState = g_pIndex->m_idxStates.GetPtrAndAddRef( in_ulStateGroupID, in_ulStateUniqueID );
	if(!pState)
	{
		return AK_InvalidInstanceID;
	}
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	AKASSERT( l_ppStateGrpInfo );
	AKASSERT( *l_ppStateGrpInfo );
	if( ( *l_ppStateGrpInfo )->mapStates.Exists( in_ulStateID ) )
	{
		RemoveState( in_ulStateGroupID, in_ulStateID );
	}

	AkStateLink * pLink = ( *l_ppStateGrpInfo )->mapStates.Set( in_ulStateID );
	if ( pLink )
	{
		pLink->pState = pState;
		pLink->ulStateID = in_ulStateUniqueID;
		pLink->bUseStateGroupInfo = false;

		pState->InitNotificationSystem(NULL);

		if( GetState(in_ulStateGroupID) == in_ulStateID )
		{
			AK::SoundEngine::SetState( in_ulStateGroupID, in_ulStateID );
		}

		return AK_Success;
	}
	else
	{
		pState->Release();
		return AK_Fail;
	}
}

AKRESULT CAkStateMgr::RemoveState(AkStateGroupID in_ulStateGroupID,AkStateID in_ulStateID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	AKASSERT( l_ppStateGrpInfo );
	AkStateGroupInfo* pInfo = *l_ppStateGrpInfo;
	AKASSERT(pInfo);
	AkStateLink* l_pStateLink = pInfo->mapStates.Exists( in_ulStateID );
	if( !l_pStateLink )
	{
		return AK_IDNotFound;
	}
	else
	{
		CAkState* pState = l_pStateLink->pState;
		if(pInfo->StatePtr == pState)
		{
			pInfo->StatePtr = NULL;
		}
		pState->TermNotificationSystem();
		pState->Release(); 
		pInfo->mapStates.Unset( in_ulStateID );
		return AK_Success;
	}
}

#ifndef AK_OPTIMIZED
AKRESULT CAkStateMgr::ResetAllStates()
{
	//TODO(alessard) to be revised...
	AKRESULT eResult = AK_Success;

	//Tells everybody to set stat to none without transition time
	for( AkListStateGroups::Iterator iter = m_StateGroups.Begin(); iter != m_StateGroups.End(); ++iter )
	{
		AK::SoundEngine::SetState( (*iter).key, 0, true, false ); // true to skip transitiontime
	}

	// This should theorically not pass trough the index, it shall use the Master bus 
	// notification system once it is available
	// But for now, let's use the indexes.
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkAudioNode*>& l_rIdx = g_pIndex->m_idxAudioNode;

	{//Bracket for autolock
		AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

		AkHashListBare<AkUniqueID, CAkIndexable, 31>::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			static_cast<CAkParameterNodeBase*>( *iter )->UseState( true );
			++iter;
		}
	}

	return eResult;
}

#endif

AKRESULT CAkStateMgr::RemoveStates(AkStateGroupID in_ulStateGroupID)
{
	AkStateGroupInfo** l_ppStateGrpInfo = m_StateGroups.Exists( in_ulStateGroupID );
	AKASSERT( l_ppStateGrpInfo );
	AkStateGroupInfo* pInfo = *l_ppStateGrpInfo;
	AKASSERT(pInfo);
	pInfo->StatePtr = NULL;

	for( AkStateGroupInfo::AkMapStates::Iterator iter = pInfo->mapStates.Begin(); iter != pInfo->mapStates.End(); ++iter )
	{
		CAkState* pState = (*iter).item.pState;
		pState->TermNotificationSystem();
		pState->Release();
	}
	pInfo->mapStates.RemoveAll();
	return AK_Success;
}

AKRESULT CAkStateMgr::RemoveAllStates()
{
	for( AkListStateGroups::Iterator iter = m_StateGroups.Begin(); iter != m_StateGroups.End(); ++iter )
	{
		AkStateGroupInfo* l_pStateGrpInfo = (*iter).item;

		AkInt32 cStates = (AkInt32) l_pStateGrpInfo->mapStates.Length();
		for ( AkInt32 iState = cStates - 1; iState >= 0; --iState )
		{
			RemoveState( (*iter).key, l_pStateGrpInfo->mapStates[ iState ].key );
		}
	}
    return AK_Success;
}

AKRESULT CAkStateMgr::RemoveAllStateGroups( bool in_bIsFromClearBanks )
{
	AkListStateGroups::Iterator iter = m_StateGroups.Begin();
	while( iter != m_StateGroups.End() )
    {
		AkStateGroupInfo* pInfo = (*iter).item;
		if( !in_bIsFromClearBanks || pInfo->listMemberNodes.IsEmpty() )
		{
			pInfo->Term();
			AkDelete( g_DefaultPoolId, pInfo );
			iter = m_StateGroups.EraseSwap( iter );
		}
		else
		{
			++iter;
		}
    }
	return AK_Success;
}

void CAkStateMgr::NotifyStateModified( AkUniqueID in_ulUniqueStateID )
{
	for( AkListStateGroups::Iterator iter = m_StateGroups.Begin(); iter != m_StateGroups.End(); ++iter )
	{
		AkStateGroupInfo* l_pStateGroupInfo = (*iter).item;
		CAkState* pState = l_pStateGroupInfo->StatePtr;
		if(pState != NULL && pState->ID() == in_ulUniqueStateID)
		{
			for( AkStateGroupInfo::AkListMemberNode::Iterator iter = l_pStateGroupInfo->listMemberNodes.Begin(); iter != l_pStateGroupInfo->listMemberNodes.End(); ++iter )
			{
				(*iter)->NotifyStateParametersModified();
			}
			break;
		}
	}
}

AKRESULT CAkStateMgr::RegisterSwitch( CAkSwitchAware* in_pSwitchCntr, AkStateGroupID in_ulStateGroup )
{
	AKRESULT eResult = AK_Success;
	AKASSERT( in_pSwitchCntr );
	if(in_pSwitchCntr)
	{
		CAkStateMgr::RegisteredSwitch l_RS;
		l_RS.pSwitch = in_pSwitchCntr;
		l_RS.ulStateGroup = in_ulStateGroup;
		if ( !m_listRegisteredSwitch.AddLast( l_RS ) )
		{
			eResult = AK_Fail;
		}
	}
	else
	{
		eResult = AK_InvalidParameter;
	}

	return eResult;
}

AKRESULT CAkStateMgr::UnregisterSwitch( CAkSwitchAware* in_pSwitchCntr )
{
	AKRESULT eResult = AK_Success;

	AkListRegisteredSwitch::IteratorEx iter = m_listRegisteredSwitch.BeginEx();
	while( iter != m_listRegisteredSwitch.End() )
	{
		if( (*iter).pSwitch == in_pSwitchCntr )
		{
			iter = m_listRegisteredSwitch.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}

CAkStateMgr::PreparationStateItem* CAkStateMgr::GetPreparationItem( AkUInt32 in_ulGroup, AkGroupType in_eGroupType )
{
	AkAutoLock<CAkLock> gate( m_PrepareGameSyncLock );// to pad monitoring recaps.

	PreparationGroups* pPreparationGroup;
	if( in_eGroupType == AkGroupType_State )
	{
		pPreparationGroup = &m_PreparationGroupsStates;
	}
	else
	{
		AKASSERT( in_eGroupType == AkGroupType_Switch );
		pPreparationGroup = &m_PreparationGroupsSwitches;
	}
	
	PreparationGroups::IteratorEx iter = pPreparationGroup->BeginEx();
	while( iter != pPreparationGroup->End() )
	{
		if( in_ulGroup == iter.pItem->GroupID() )
			return iter.pItem;
		++iter;
	}

	PreparationStateItem* pPreparationStateItem = NULL;
	AkNew2( pPreparationStateItem, g_DefaultPoolId, PreparationStateItem, PreparationStateItem( in_ulGroup ) );

	if( pPreparationStateItem )
	{
		pPreparationGroup->AddFirst( pPreparationStateItem );
	}

	return pPreparationStateItem;
}

AKRESULT CAkStateMgr::PrepareGameSync( 
		AkGroupType in_eGroupType, 
		AkUInt32 in_uGroupID, 
		AkUInt32 in_uGameSyncID, 
		bool in_bIsActive 
		)
{
	CAkStateMgr::PreparationStateItem* pPreparationStateItem = GetPreparationItem( in_uGroupID, in_eGroupType );
	if( !pPreparationStateItem )
	{
		// If is was going to be removed, not finding it is not an issue, return success.
		if( in_bIsActive )
			return AK_InsufficientMemory;
		else
			return AK_Success;
	}

	CAkPreparedContent::ContentList& rContentList = pPreparationStateItem->GetPreparedcontent()->m_PreparableContentList;

	CAkPreparedContent::ContentList::Iterator iter = rContentList.FindEx( in_uGameSyncID );

	if( iter != rContentList.End() )
	{
		if( in_bIsActive )
			return AK_Success; // nothing to do, everything is already alright.
		rContentList.EraseSwap( iter );
	}
	else
	{
		if( !in_bIsActive )
			return AK_Success; // nothing to do, everything is already alright.
		rContentList.AddLast( in_uGameSyncID );
	}

	AKRESULT eResult = pPreparationStateItem->Notify( in_uGameSyncID, in_bIsActive );
	if( eResult != AK_Success )
	{
		// If this assert pops. it means that we encountered an error while trying to free loaded memory.
		AKASSERT( in_bIsActive && "Unhandled situation, PrepareGameSync(...) failed to free allocated data" );
		rContentList.EraseSwap( iter );
	}
	else
	{
		MONITOR_GAMESYNC( in_uGroupID, in_uGameSyncID, in_bIsActive, in_eGroupType );
	}

	return eResult;
}

AKRESULT CAkStateMgr::UpdateSwitches( AkStateGroupID in_ulStateGroup, AkStateID in_StateFrom, AkStateID in_StateTo )
{
	AKRESULT eResult = AK_Success;

	if( in_StateFrom != in_StateTo )
	{
		for( AkListRegisteredSwitch::Iterator iter = m_listRegisteredSwitch.Begin(); iter != m_listRegisteredSwitch.End(); ++iter )
		{
			RegisteredSwitch& l_rRegSw = *iter;
			if( l_rRegSw.ulStateGroup == in_ulStateGroup )
			{
				AKASSERT( l_rRegSw.pSwitch );
				l_rRegSw.pSwitch->SetSwitch( in_StateTo );
			}
		}
	}

	return eResult;
}

AKRESULT CAkStateMgr::RegisterTrigger( IAkTriggerAware* in_pTrigerAware, AkTriggerID in_triggerID, CAkRegisteredObj* in_GameObj )
{
	AKASSERT( in_pTrigerAware );

	RegisteredTrigger	l_RT;
	l_RT.pTriggerAware	= in_pTrigerAware;
	l_RT.triggerID		= in_triggerID;
	l_RT.gameObj		= in_GameObj;

	if ( !m_listRegisteredTrigger.AddLast( l_RT ) )
		return AK_Fail;

	return AK_Success;
}

AKRESULT CAkStateMgr::UnregisterTrigger( IAkTriggerAware* in_pTrigerAware, AkTriggerID in_Trigger )
{
	AkListRegisteredTrigger::IteratorEx iter = m_listRegisteredTrigger.BeginEx();
	while( iter != m_listRegisteredTrigger.End() )
	{
		if( (*iter).pTriggerAware == in_pTrigerAware && (*iter).triggerID == in_Trigger )
		{
			iter = m_listRegisteredTrigger.Erase( iter );
			break;
		}
		else
		{
			++iter;
		}
	}

	return AK_Success;
}

AKRESULT CAkStateMgr::RegisterTrigger( IAkTriggerAware* in_pTrigerAware, CAkRegisteredObj* in_GameObj )
{
	AKASSERT( in_pTrigerAware );

	RegisteredTrigger	l_RT;
	l_RT.pTriggerAware	= in_pTrigerAware;
	l_RT.triggerID		= AK_INVALID_UNIQUE_ID;
	l_RT.gameObj		= in_GameObj;

	if ( !m_listRegisteredTrigger.AddLast( l_RT ) )
		return AK_Fail;

	return AK_Success;
}

AKRESULT CAkStateMgr::UnregisterTrigger( IAkTriggerAware* in_pTrigerAware )
{
    AkListRegisteredTrigger::IteratorEx iter = m_listRegisteredTrigger.BeginEx();
	while( iter != m_listRegisteredTrigger.End() )
	{
		if( (*iter).pTriggerAware == in_pTrigerAware )
		{
			iter = m_listRegisteredTrigger.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return AK_Success;
}

void CAkStateMgr::Trigger( AkTriggerID in_Trigger, CAkRegisteredObj* in_GameObj )
{
	for( AkListRegisteredTrigger::Iterator iter = m_listRegisteredTrigger.Begin(); iter != m_listRegisteredTrigger.End(); ++iter )
	{
		RegisteredTrigger& l_rRegTrigger = *iter;
		if( l_rRegTrigger.triggerID == AK_INVALID_UNIQUE_ID || l_rRegTrigger.triggerID == in_Trigger )
		{
			if( in_GameObj == NULL || l_rRegTrigger.gameObj == in_GameObj )
			{
				l_rRegTrigger.pTriggerAware->Trigger( in_Trigger );
			}
		}
	}
}
