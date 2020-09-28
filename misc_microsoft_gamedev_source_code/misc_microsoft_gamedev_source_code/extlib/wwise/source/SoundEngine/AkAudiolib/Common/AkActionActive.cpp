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
// AkActionActive.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionActive.h"
#include "AkAudioLibIndex.h"
#include "AkAudioNode.h"
#include "AkPBI.h"
#include "AkRegistryMgr.h"
#include "AkAudioMgr.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"
#include "AkBus.h"
#include "AkDynamicSequence.h"

extern CAkAudioMgr* g_pAudioMgr;
extern CAkBus * g_pMasterBus;

CAkActionActive::CAkActionActive(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkActionExcept(in_eActionType, in_ulID)
, m_eFadeCurve(AkCurveInterpolation_Linear)
, m_bIsMasterResume(false)
{
}

CAkActionActive::~CAkActionActive()
{
}

void CAkActionActive::TransitionTime(const AkTimeMs in_lTransitionTime, const AkTimeMs in_RangeMin/*=0*/, const AkTimeMs in_RangeMax/*=0*/)
{
	RandomizerModifier::SetModValue( m_TransitionTime, in_lTransitionTime, in_RangeMin, in_RangeMax );
}

AKRESULT CAkActionActive::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkTimeMs TTime = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMin = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMax = READBANKDATA(AkTimeMs, io_rpData, io_rulDataSize );

	AkUInt8 ucFadeCurveType = READBANKDATA(AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;
	RandomizerModifier::SetModValue( m_TransitionTime, TTime, TTimeMin, TTimeMax );

	eResult = SetActionSpecificParams(io_rpData, io_rulDataSize);

	if(eResult == AK_Success)
	{
		eResult = SetExceptParams( io_rpData, io_rulDataSize );
	}

	return AK_Success;
}

AKRESULT CAkActionActive::Exec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj )
{
	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID);
	if(pNode)
	{
		ActionParams l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterCall = false;
		l_Params.bIsMasterResume = m_bIsMasterResume;
		l_Params.transParams.eFadeCurve = m_eFadeCurve;
		l_Params.eType = in_eType;
		l_Params.pGameObj = in_pGameObj;
		l_Params.playingID = AK_INVALID_PLAYING_ID;
		l_Params.transParams.TransitionTime = RandomizerModifier::GetModValue( m_TransitionTime );

		AKRESULT eResult = pNode->ExecuteAction( l_Params );

		pNode->Release();

		return eResult;
	}
	else
	{
		return AK_IDNotFound;
	}
}

void CAkActionActive::AllExec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj )
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkDynamicSequence*>& l_rIdx = g_pIndex->m_idxDynamicSequences;

	{	//Bracket for autolock
		AkAutoLock<CAkLock> IndexLock( l_rIdx.m_IndexLock );

		AkHashListBare<AkUniqueID, CAkIndexable, 31>::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
		while( iter != l_rIdx.m_mapIDToPtr.End() )
		{
			static_cast<CAkDynamicSequence*>( *iter )->AllExec( in_eType, in_pGameObj );
			++iter;
		}
	}

	if( g_pMasterBus )
	{
		ActionParams l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterResume = m_bIsMasterResume;
		l_Params.transParams.eFadeCurve = m_eFadeCurve;
		l_Params.eType = in_eType;
		l_Params.pGameObj = in_pGameObj;
		l_Params.playingID = AK_INVALID_PLAYING_ID;
		l_Params.transParams.TransitionTime = RandomizerModifier::GetModValue( m_TransitionTime );

		if( l_Params.pGameObj == NULL )
			l_Params.bIsMasterCall = true;
		else
			l_Params.bIsMasterCall = false;

		g_pMasterBus->ExecuteAction( l_Params );
		if (CAkFeedbackBus::GetMasterBus() != NULL)
			CAkFeedbackBus::GetMasterBus()->ExecuteAction(l_Params);
	}
}

void CAkActionActive::AllExecExcept( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj )
{
	if( g_pMasterBus )
	{
		ActionParamsExcept l_Params;
		l_Params.bIsFromBus = false;
		l_Params.bIsMasterResume = m_bIsMasterResume;
        l_Params.transParams.eFadeCurve = m_eFadeCurve;
		l_Params.eType = in_eType;
		l_Params.pGameObj = in_pGameObj;
		l_Params.pExeceptionList = &m_listElementException;
		l_Params.transParams.TransitionTime = RandomizerModifier::GetModValue( m_TransitionTime );

		g_pMasterBus->ExecuteActionExcept( l_Params );
		if (CAkFeedbackBus::GetMasterBus() != NULL)
			CAkFeedbackBus::GetMasterBus()->ExecuteActionExcept(l_Params);
	}
}
