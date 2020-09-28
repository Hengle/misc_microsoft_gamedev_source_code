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
// AkAction.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAction.h"
#include "AkAudioLibIndex.h"
#include "AkActions.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"
#ifndef AK_OPTIMIZED
#include "AkActionEvent.h"
#endif

CAkAction::CAkAction(AkActionType in_eActionType, AkUniqueID in_ulID)
:CAkIndexable(in_ulID)
,m_eActionType(in_eActionType)
,m_ulElementID(0)
{
	//MUST STAY EMPTY, use Init() instead.
}

CAkAction::~CAkAction()
{
}

CAkAction* CAkAction::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{

	switch( in_eActionType & ACTION_TYPE_ACTION )
	{
	case ACTION_TYPE_PLAY:
		return CAkActionPlay::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_STOP:
		return CAkActionStop::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_PAUSE:
		return CAkActionPause::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_RESUME:
		return CAkActionResume::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETPITCH:
	case ACTION_TYPE_RESETPITCH:
		return CAkActionSetPitch::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETVOLUME:
	case ACTION_TYPE_RESETVOLUME:
		return CAkActionSetVolume::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETLFE:
	case ACTION_TYPE_RESETLFE:
		return CAkActionSetLFE::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETLPF:
	case ACTION_TYPE_RESETLPF:
		return CAkActionSetLPF::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_MUTE:
	case ACTION_TYPE_UNMUTE:
		return CAkActionMute::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETSTATE:
		return CAkActionSetState::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_USESTATE:
	case ACTION_TYPE_UNUSESTATE:
		return CAkActionUseState::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETSWITCH:
		return CAkActionSetSwitch::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_SETRTPC:
		return CAkActionSetRTPC::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_BYPASSFX:
	case ACTION_TYPE_RESETBYPASSFX:
		return CAkActionBypassFX::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_BREAK:
		return CAkActionBreak::Create(in_eActionType, in_ulID);

	case ACTION_TYPE_TRIGGER:
		return CAkActionTrigger::Create( in_eActionType, in_ulID );

#ifndef AK_OPTIMIZED
	case ACTION_TYPE_STOPEVENT:
	case ACTION_TYPE_PAUSEEVENT:	
	case ACTION_TYPE_RESUMEEVENT:
		return CAkActionEvent::Create(in_eActionType, in_ulID);

#endif
	default:
		AKASSERT(!"Unknown Action Type");
		return NULL;
	}
}

void CAkAction::AddToIndex()
{
	AKASSERT(g_pIndex);
	if( ID() != AK_INVALID_UNIQUE_ID ) // Check if the action has an ID, it does not have an ID if was created by the sound engine
		g_pIndex->m_idxActions.SetIDToPtr( this );
}

void CAkAction::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	if( ID() != AK_INVALID_UNIQUE_ID ) // Check if the action has an ID, it does not have an ID if was created by the sound engine
		g_pIndex->m_idxActions.RemoveID( ID() );
}

AkUInt32 CAkAction::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxActions.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkAction::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxActions.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex(); 
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

void CAkAction::ActionType(AkActionType in_ActionType)
{
	AKASSERT( !"Unpredictable behavior to change the action tupe of this Action" );
	m_eActionType = in_ActionType;
}

void CAkAction::SetElementID(AkUniqueID in_ulElementID)
{
	m_ulElementID = in_ulElementID;
}

void CAkAction::Delay(AkInt32 in_Delay, AkInt32 in_RangeMin/*=0*/, AkInt32 in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_Delay, in_Delay, in_RangeMin, in_RangeMax );
}

AkInt32 CAkAction::Delay()
{
	return RandomizerModifier::GetModValue( m_Delay );
}

AKRESULT CAkAction::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	// Read ID
	// We don't care about the ID, just skip it
	AkUInt32 ulID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize);

	AkActionType ulActionType = READBANKDATA( AkActionType, in_pData, in_ulDataSize);

	AKASSERT(m_eActionType == ulActionType);

	{
		AkUInt32 ulTargetID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize);

		SetElementID(ulTargetID);
	}

	{
		AkTimeMs tDelay = READBANKDATA( AkTimeMs, in_pData, in_ulDataSize);

		AkTimeMs tDelayMin = READBANKDATA( AkTimeMs, in_pData, in_ulDataSize);

		AkTimeMs tDelayMax = READBANKDATA( AkTimeMs, in_pData, in_ulDataSize);

		Delay(
			CAkTimeConv::MillisecondsToSamples( tDelay ), 
			CAkTimeConv::MillisecondsToSamples( tDelayMin ), 
			CAkTimeConv::MillisecondsToSamples( tDelayMax ) );
	}

	{
		AkUInt32 ulSubSectionSize = READBANKDATA( AkUInt32, in_pData, in_ulDataSize);

		if(ulSubSectionSize)
		{
			eResult = SetActionParams(in_pData, in_ulDataSize);
		}
	}
	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkAction::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// If this function is called it should be because no parameters need to be processed
	// No need to assert, it is not an error, ignoring the remaining info to read
	io_rulDataSize = 0;
	return AK_Success;
}

AKRESULT CAkAction::SetExceptParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// Simply skip it since we don't have Exceptions for this type of action
	AkUInt32 ulExceptionListSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	AkUInt32 ulID;
	for (AkUInt32 i = 0; i < ulExceptionListSize; ++i)
	{
		AKASSERT(!" Found Excetption somewhere there should not be exception, ignoring it");
		ulID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
	}
	return AK_Success;
}

AKRESULT CAkAction::SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// Ignoring the 4 target values
	io_rpData += 4 * sizeof(AkUInt32);
	io_rulDataSize -= 4 * sizeof(AkUInt32);

	return AK_Success;
}
