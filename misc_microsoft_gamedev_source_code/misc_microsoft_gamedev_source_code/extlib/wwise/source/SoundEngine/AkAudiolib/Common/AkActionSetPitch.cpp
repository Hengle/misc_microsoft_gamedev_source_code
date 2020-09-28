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
// AkActionSetPitch.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionSetPitch.h"
#include "AkAudioLibIndex.h"
#include "AkParameterNode.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionSetPitch::CAkActionSetPitch(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionSetValue(in_eActionType, in_ulID)
, m_eValueMeaning( AkValueMeaning_Offset )
{
}

CAkActionSetPitch::~CAkActionSetPitch()
{

}

void CAkActionSetPitch::ExecSetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->SetPitch( NULL,m_eValueMeaning, RandomizerModifier::GetModValue( m_TargetValue ) ,m_eFadeCurve, RandomizerModifier::GetModValue( m_TransitionTime ) );
}
void CAkActionSetPitch::ExecSetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->SetPitch( in_pGameObj,m_eValueMeaning,RandomizerModifier::GetModValue( m_TargetValue ),m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}

void CAkActionSetPitch::ExecResetValue(CAkParameterNodeBase* in_pNode)
{
	in_pNode->ResetPitch( m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}
void CAkActionSetPitch::ExecResetValue(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj)
{
	in_pNode->SetPitch( in_pGameObj,AkValueMeaning_Default,0,m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}

void CAkActionSetPitch::ExecResetValueAll(CAkParameterNodeBase* in_pNode)
{
	in_pNode->ResetPitch( m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}

void CAkActionSetPitch::ExecResetValueExcept(CAkParameterNodeBase* in_pNode)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->ResetPitch( m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}

void CAkActionSetPitch::ExecResetValueExcept(CAkParameterNodeBase* in_pNode,CAkRegisteredObj * in_pGameObj)
{
	for( ExceptionList::Iterator iter = m_listElementException.Begin(); iter != m_listElementException.End(); ++iter )
	{
		if( (*iter) == in_pNode->ID() )
		{
			return;
		}
	}
	in_pNode->SetPitch( in_pGameObj,AkValueMeaning_Default,0,m_eFadeCurve, RandomizerModifier::GetModValue(m_TransitionTime) );
}

CAkActionSetPitch* CAkActionSetPitch::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionSetPitch* pActionSetPitch = AkNew(g_DefaultPoolId,CAkActionSetPitch(in_eActionType, in_ulID));
	if( pActionSetPitch )
	{
		if( pActionSetPitch->Init() != AK_Success )
		{
			pActionSetPitch->Release();
			pActionSetPitch = NULL;
		}
	}

	return pActionSetPitch;
}

void CAkActionSetPitch::SetValue(const AkPitchValue in_Value, const AkValueMeaning in_eValueMeaning, const AkPitchValue in_RangeMin/*=0*/, const AkPitchValue in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TargetValue, in_Value, in_RangeMin, in_RangeMax);
	m_eValueMeaning = in_eValueMeaning;
}

void CAkActionSetPitch::ActionType(AkActionType in_ActionType)
{
	AKASSERT(
		in_ActionType == AkActionType_SetPitch_M ||
		in_ActionType == AkActionType_SetPitch_O ||
		in_ActionType == AkActionType_ResetPitch_AE ||
		in_ActionType == AkActionType_ResetPitch_AE_O ||
		in_ActionType == AkActionType_ResetPitch_ALL ||
		in_ActionType == AkActionType_ResetPitch_ALL_O ||
		in_ActionType == AkActionType_ResetPitch_M ||
		in_ActionType == AkActionType_ResetPitch_O
	);
	m_eActionType = in_ActionType;
}

AKRESULT CAkActionSetPitch::SetActionSpecificParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 TargetValueMeaning = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	AkUInt32 Pitch = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	AkUInt32 PitchMin = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	AkUInt32 PitchMax = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	RandomizerModifier::SetModValue<AkPitchValue>( m_TargetValue, Pitch, PitchMin, PitchMax );
	m_eValueMeaning = AkValueMeaning( TargetValueMeaning );

	return AK_Success;
}
