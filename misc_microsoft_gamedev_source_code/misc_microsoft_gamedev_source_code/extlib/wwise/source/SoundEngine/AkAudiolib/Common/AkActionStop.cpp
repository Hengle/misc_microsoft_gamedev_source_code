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
// AkActionStop.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionStop.h"
#include "AkAudioLibIndex.h"
#include "AkAudioNode.h"
#include "AkPBI.h"
#include "AkModifiers.h"
#include "AkAudioMgr.h"

extern AkMemPoolId g_DefaultPoolId;
extern CAkAudioMgr* g_pAudioMgr;

CAkActionStop::CAkActionStop(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionActive(in_eActionType, in_ulID)
{
}

CAkActionStop::~CAkActionStop()
{

}

CAkActionStop* CAkActionStop::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionStop* pActionStop = AkNew(g_DefaultPoolId,CAkActionStop(in_eActionType, in_ulID));
	if( pActionStop )
	{
		if( pActionStop->Init() != AK_Success )
		{
			pActionStop->Release();
			pActionStop = NULL;
		}
	}

	return pActionStop;
}

AKRESULT CAkActionStop::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Stop_E:
			g_pAudioMgr->RemovePausedPendingAction(m_ulElementID);
			g_pAudioMgr->RemovePendingAction(m_ulElementID);
			// no break

		case AkActionType_Stop_E_O:
			eResult = Exec( ActionParamType_Stop, pGameObj );
			g_pAudioMgr->StopPendingAction( m_ulElementID, pGameObj );
			break;

		case AkActionType_Stop_ALL_O:
		case AkActionType_Stop_ALL:
			AllExec( ActionParamType_Stop, pGameObj );
			g_pAudioMgr->StopPendingAction( m_ulElementID, pGameObj );
			break;

		case AkActionType_Stop_AE_O:
		case AkActionType_Stop_AE:
			AllExecExcept( ActionParamType_Stop, pGameObj );
			g_pAudioMgr->StopPendingActionAllExcept( pGameObj, &m_listElementException );
			break;

		default:
			AKASSERT(!"Should not happen, unsupported Stop condition");
			break;
	}
	return eResult;
}

void CAkActionStop::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Stop_E ||
		in_ActionType == AkActionType_Stop_E_O ||
												
		in_ActionType == AkActionType_Stop_ALL ||
		in_ActionType == AkActionType_Stop_ALL_O ||
												
		in_ActionType == AkActionType_Stop_AE ||
		in_ActionType == AkActionType_Stop_AE_O 
	);
	m_eActionType = in_ActionType;
}
