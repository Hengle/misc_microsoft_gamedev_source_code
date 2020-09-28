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
// AkActionMute.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionMute.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNode.h"
#include "AkModifiers.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionMute::CAkActionMute(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionSetValue(in_eActionType, in_ulID)
{
}

CAkActionMute::~CAkActionMute()
{

}

void CAkActionMute::ExecSetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->Mute( NULL, m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}
void CAkActionMute::ExecSetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->Mute( in_pGameObj, m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionMute::ExecResetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->Unmute( NULL, m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}
void CAkActionMute::ExecResetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->Unmute( in_pGameObj, m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionMute::ExecResetValueExcept(CAkParameterNodeBase* in_pNode)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->UnmuteAll( m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionMute::ExecResetValueExcept( CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj )
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->Unmute( in_pGameObj, m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionMute::ExecResetValueAll(CAkParameterNodeBase* in_pNode)
{
	in_pNode->UnmuteAll( m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

CAkActionMute* CAkActionMute::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionMute* pActionMute = AkNew(g_DefaultPoolId,CAkActionMute(in_eActionType, in_ulID));
	if( pActionMute )
	{
		if( pActionMute->Init() != AK_Success )
		{
			pActionMute->Release();
			pActionMute = NULL;
		}
	}

	return pActionMute;
}

void CAkActionMute::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_Mute_M ||
		in_ActionType == AkActionType_Mute_O ||
		in_ActionType == AkActionType_Unmute_AE ||
		in_ActionType == AkActionType_Unmute_AE_O ||
		in_ActionType == AkActionType_Unmute_ALL ||
		in_ActionType == AkActionType_Unmute_ALL_O ||
		in_ActionType == AkActionType_Unmute_M ||
		in_ActionType == AkActionType_Unmute_O
	);
	m_eActionType = in_ActionType;
}
