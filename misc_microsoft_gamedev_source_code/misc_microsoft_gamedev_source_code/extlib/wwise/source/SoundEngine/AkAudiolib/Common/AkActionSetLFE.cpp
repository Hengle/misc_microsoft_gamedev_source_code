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
// AkActionSetLFE.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetLFE.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionSetLFE::CAkActionSetLFE(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionSetValue(in_eActionType, in_ulID)
, m_eValueMeaning( AkValueMeaning_Offset )
{
}

CAkActionSetLFE::~CAkActionSetLFE()
{

}

void CAkActionSetLFE::ExecSetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->SetLFE( NULL,m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ), m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime));
}
void CAkActionSetLFE::ExecSetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->SetLFE( in_pGameObj,m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ), m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionSetLFE::ExecResetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->ResetLFE(m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}
void CAkActionSetLFE::ExecResetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->SetLFE( in_pGameObj,AkValueMeaning_Default,0.0f,m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionSetLFE::ExecResetValueAll(CAkParameterNodeBase* in_pNode)
{
	in_pNode->ResetLFE(m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionSetLFE::ExecResetValueExcept(CAkParameterNodeBase* in_pNode)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->ResetLFE(m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

void CAkActionSetLFE::ExecResetValueExcept(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj )
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->SetLFE( in_pGameObj,AkValueMeaning_Default,0.0f,m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}

CAkActionSetLFE* CAkActionSetLFE::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionSetLFE*	pActionSetLFE = AkNew(g_DefaultPoolId,CAkActionSetLFE(in_eActionType, in_ulID));
	if( pActionSetLFE )
	{
		if( pActionSetLFE->Init() != AK_Success )
		{
			pActionSetLFE->Release();
			pActionSetLFE = NULL;
		}
	}

	return pActionSetLFE;
}

void CAkActionSetLFE::SetValue(const AkReal32 in_fValue, const AkValueMeaning in_eValueMeaning, const AkReal32 in_RangeMin/*=0*/, const AkReal32 in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TargetValue, in_fValue, in_RangeMin, in_RangeMax);
	m_eValueMeaning = in_eValueMeaning;
}

void CAkActionSetLFE::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_SetLFE_M ||
		in_ActionType == AkActionType_SetLFE_O ||
		in_ActionType == AkActionType_ResetLFE_AE ||
		in_ActionType == AkActionType_ResetLFE_AE_O ||
		in_ActionType == AkActionType_ResetLFE_ALL ||
		in_ActionType == AkActionType_ResetLFE_ALL_O ||
		in_ActionType == AkActionType_ResetLFE_M ||
		in_ActionType == AkActionType_ResetLFE_O
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionSetLFE::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 TargetValueMeaning = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	AkReal32 ValBase = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMin = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMax = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	RandomizerModifier::SetModValue( m_TargetValue, ValBase, ValMin, ValMax );
	m_eValueMeaning = AkValueMeaning( TargetValueMeaning );

	return AK_Success;
}
