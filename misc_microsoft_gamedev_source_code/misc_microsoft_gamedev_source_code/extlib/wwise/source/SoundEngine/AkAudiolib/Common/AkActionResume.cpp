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
// AkActionPause.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionResume.h"
#include "AkAudioLibIndex.h"
#include "AkAudioNode.h"
#include "AkPBI.h"
#include "AkModifiers.h"
#include "AkAudioMgr.h"
#include "AkBankFloatConversion.h"

extern AkMemPoolId g_DefaultPoolId;
extern CAkAudioMgr* g_pAudioMgr;

CAkActionResume::CAkActionResume(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionActive(in_eActionType, in_ulID)
{
}

CAkActionResume::~CAkActionResume()
{

}

CAkActionResume* CAkActionResume::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionResume* pActionResume = AkNew(g_DefaultPoolId,CAkActionResume(in_eActionType, in_ulID));
	if( pActionResume )
	{
		if( pActionResume->Init() != AK_Success )
		{
			pActionResume->Release();
			pActionResume = NULL;
		}
	}

	return pActionResume;
}

AKRESULT CAkActionResume::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pAudioMgr);

	AKRESULT eResult = AK_Success;

	CAkRegisteredObj * pGameObj = in_pAction->GameObj();

	switch(CAkAction::ActionType())
	{
		case AkActionType_Resume_E_O:
		case AkActionType_Resume_E:
			eResult = Exec( ActionParamType_Resume, pGameObj );
			g_pAudioMgr->ResumePausedPendingAction( m_ulElementID, pGameObj, m_bIsMasterResume );
			g_pAudioMgr->ResumeNotPausedPendingAction( m_ulElementID, pGameObj, m_bIsMasterResume );
			break;

		case AkActionType_Resume_ALL_O:
		case AkActionType_Resume_ALL:
			AllExec( ActionParamType_Resume, pGameObj );
			g_pAudioMgr->ResumePausedPendingAction( m_ulElementID, pGameObj, m_bIsMasterResume );
			g_pAudioMgr->ResumeNotPausedPendingAction( m_ulElementID, pGameObj, m_bIsMasterResume );
			break;

		case AkActionType_Resume_AE_O:
		case AkActionType_Resume_AE:
			AllExecExcept( ActionParamType_Resume, pGameObj );
			g_pAudioMgr->ResumePausedPendingActionAllExcept( pGameObj, &m_listElementException, m_bIsMasterResume );
			g_pAudioMgr->ResumeNotPausedPendingActionAllExcept( pGameObj, &m_listElementException, m_bIsMasterResume );
			break;

		default:
			AKASSERT(!"Should not happen, unsupported condition");
			break;
	}
	return eResult;
}

void CAkActionResume::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Resume_E ||
		in_ActionType == AkActionType_Resume_E_O ||
												
		in_ActionType == AkActionType_Resume_ALL ||
		in_ActionType == AkActionType_Resume_ALL_O ||
												
		in_ActionType == AkActionType_Resume_AE ||
		in_ActionType == AkActionType_Resume_AE_O 
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionResume::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 b32IsMaster = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize);
	IsMasterResume( b32IsMaster != 0 );
	// Ignoring the 3 target values
	io_rpData += 3 * sizeof(AkUInt32);
	io_rulDataSize -= 3 * sizeof(AkUInt32);

	return AK_Success;
}
