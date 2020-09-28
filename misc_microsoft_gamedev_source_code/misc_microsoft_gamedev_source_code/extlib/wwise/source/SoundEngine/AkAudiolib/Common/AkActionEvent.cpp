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
// AkActionEvent.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkActionEvent.h"
#include "AkEvent.h"
#include "AkAudiolibIndex.h"
#include "AkAudioMgr.h"
#include "AkParameterNode.h"
#include "AkRegisteredObj.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionEvent::CAkActionEvent(AkActionType in_eActionType, AkUniqueID in_ulID)
: CAkAction(in_eActionType, in_ulID)
, m_ulTargetEventID( 0 )
{
}

CAkActionEvent::~CAkActionEvent()
{
}

void CAkActionEvent::Delay( AkTimeMs in_Delay, AkTimeMs in_RangeMin, AkTimeMs in_RangeMax )
{
	//NEVER try to delay a cancelation of event NEVER!!!!!!!!!!!!!!!!!!!!!!
	AKASSERT(!"Trying to delay a non delayable action");
}

AKRESULT CAkActionEvent::Execute( AkPendingAction * in_pAction )
{
	AKRESULT eResult = AK_Success;
	AKASSERT(g_pIndex);

	CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( m_ulTargetEventID );
	
	if( pEvent )
	{
		CAkRegisteredObj * pGameObj = in_pAction->GameObj();

		CAkEvent::AkActionList::Iterator iter = pEvent->m_actions.Begin();
		while( iter != pEvent->m_actions.End() )
		{
			CAkAction* pAction = *iter;
			if ( pAction->ActionType() == AkActionType_Play )
			{
				CAkAudioNode* pNode = static_cast<CAkAudioNode*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(pAction->ElementID()));
				if(m_eActionType == AkActionType_StopEvent)
				{
					g_pAudioMgr->StopPendingAction( pAction->ElementID(), pGameObj );
					if(pNode)
						pNode->Stop( pGameObj );
				}
				else if(m_eActionType == AkActionType_PauseEvent)
				{
					g_pAudioMgr->PausePendingAction( pAction->ElementID(), pGameObj, true );
					if(pNode)
						pNode->Pause( pGameObj );
				}
				else
				{
					g_pAudioMgr->ResumePausedPendingAction( pAction->ElementID(), pGameObj, false );
					g_pAudioMgr->ResumeNotPausedPendingAction( pAction->ElementID(), pGameObj, false );
					if(pNode)
						pNode->Resume( pGameObj );
				}
				if ( pNode )
					pNode->Release();
			}
			else
			{
				if(m_eActionType == AkActionType_StopEvent)
				{
					g_pAudioMgr->StopAction( pAction->ID() );
				}
				else if(m_eActionType == AkActionType_PauseEvent)
				{
					g_pAudioMgr->PauseAction( pAction->ID() );
				}
				else
				{
					g_pAudioMgr->ResumeAction( pAction->ID() );
				}
			}
			++iter;
		}
		pEvent->Release();
	}
	return eResult;
}

void CAkActionEvent::SetElementID( AkUniqueID in_ulElementID )
{
	m_ulTargetEventID = in_ulElementID;
}

void CAkActionEvent::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_StopEvent ||
		in_ActionType == AkActionType_PauseEvent ||										
		in_ActionType == AkActionType_ResumeEvent
	);
	m_eActionType = in_ActionType;
}

CAkActionEvent* CAkActionEvent::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionEvent*	pActionEvent = AkNew(g_DefaultPoolId,CAkActionEvent(in_eActionType, in_ulID));
	if( pActionEvent )
	{
		if( pActionEvent->Init() != AK_Success )
		{
			pActionEvent->Release();
			pActionEvent = NULL;
		}
	}

	return pActionEvent;
}
