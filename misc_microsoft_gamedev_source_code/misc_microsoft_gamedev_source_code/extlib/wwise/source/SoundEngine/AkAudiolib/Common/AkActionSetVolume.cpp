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
// AkActionSetVolume.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetVolume.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionSetVolume::CAkActionSetVolume(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionSetValue(in_eActionType, in_ulID)
, m_eValueMeaning( AkValueMeaning_Offset )
{
}

CAkActionSetVolume::~CAkActionSetVolume()
{

}

void CAkActionSetVolume::ExecSetValue(CAkParameterNodeBase* in_pNode)
{
	ExecSetValueInternal(in_pNode, NULL, m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ));
}

void CAkActionSetVolume::ExecSetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	ExecSetValueInternal(in_pNode, in_pGameObj, m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ));
}

void CAkActionSetVolume::ExecResetValue(CAkParameterNodeBase* in_pNode)
{
	AkInt32 lTime = RandomizerModifier::GetModValue( m_TransitionTime );
	in_pNode->ResetVolume( m_eFadeCurve, lTime );
}

void CAkActionSetVolume::ExecResetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	ExecSetValueInternal(in_pNode, in_pGameObj, AkValueMeaning_Default, 0.0f );
}

void CAkActionSetVolume::ExecResetValueAll(CAkParameterNodeBase* in_pNode)
{
	ExecResetValue(in_pNode);
}

void CAkActionSetVolume::ExecResetValueExcept(CAkParameterNodeBase* in_pNode)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}

	ExecResetValue(in_pNode);
}

void CAkActionSetVolume::ExecResetValueExcept(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj )
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	ExecResetValue(in_pNode, in_pGameObj);
}

void CAkActionSetVolume::ExecSetValueInternal(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj, AkValueMeaning in_eMeaning, AkReal32 in_fValue)
{
	AkInt32 lTime = RandomizerModifier::GetModValue( m_TransitionTime );
	in_pNode->SetVolume( in_pGameObj, in_eMeaning, in_fValue, m_eFadeCurve, lTime );
}

CAkActionSetVolume* CAkActionSetVolume::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionSetVolume*	pActionSetVolume = AkNew(g_DefaultPoolId,CAkActionSetVolume(in_eActionType, in_ulID));
	if( pActionSetVolume )
	{
		if( pActionSetVolume->Init() != AK_Success )
		{
			pActionSetVolume->Release();
			pActionSetVolume = NULL;
		}
	}

	return pActionSetVolume;
}

void CAkActionSetVolume::SetValue(const AkReal32 in_fValue, const AkValueMeaning in_eValueMeaning, const AkReal32 in_RangeMin/*=0*/, const AkReal32 in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TargetValue, in_fValue, in_RangeMin, in_RangeMax);
	m_eValueMeaning = in_eValueMeaning;
}

void CAkActionSetVolume::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_SetVolume_M ||
		in_ActionType == AkActionType_SetVolume_O ||
		in_ActionType == AkActionType_ResetVolume_AE ||
		in_ActionType == AkActionType_ResetVolume_AE_O ||
		in_ActionType == AkActionType_ResetVolume_ALL ||
		in_ActionType == AkActionType_ResetVolume_ALL_O ||
		in_ActionType == AkActionType_ResetVolume_M ||
		in_ActionType == AkActionType_ResetVolume_O
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionSetVolume::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 TargetValueMeaning = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	AkReal32 ValBase = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMin = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	AkReal32 ValMax = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

	RandomizerModifier::SetModValue( m_TargetValue, ValBase, ValMin, ValMax );
	m_eValueMeaning = AkValueMeaning( TargetValueMeaning );

	return AK_Success;
}
