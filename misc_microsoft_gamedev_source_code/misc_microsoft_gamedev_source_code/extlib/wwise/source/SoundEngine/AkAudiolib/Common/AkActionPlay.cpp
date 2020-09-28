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
// AkActionPlay.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkActionPlay.h"
#include "AkPBI.h"
#include "AkAudiolibIndex.h"
#include "AkAudioNode.h"
#include "AkMonitor.h"
#include "AkBankFloatConversion.h"
#include "AkCntrHistory.h"
#include "AkModifiers.h"

#include "AkAudioMgr.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionPlay::CAkActionPlay(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkAction(in_eActionType, in_ulID)
, m_eFadeCurve(AkCurveInterpolation_Linear)
, m_fileID( AK_INVALID_FILE_ID )
{
}

CAkActionPlay::~CAkActionPlay()
{
	
}

AKRESULT CAkActionPlay::Execute( AkPendingAction * in_pAction )
{
	AKRESULT eResult = AK_Success;
	AKASSERT(g_pIndex);
	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_ulElementID );

	if(pNode)
	{
		TransParams	Tparameters;

		Tparameters.TransitionTime = RandomizerModifier::GetModValue( m_TransitionTime );
		Tparameters.eFadeCurve = (AkCurveInterpolation)m_eFadeCurve;

		AkPBIParams pbiParams;
        
		pbiParams.eType = AkPBIParams::PBI;
        pbiParams.pInstigator = pNode;
		pbiParams.userParams = in_pAction->UserParam;
		pbiParams.ePlaybackState = PB_Playing;
		pbiParams.uFrameOffset = in_pAction->LaunchFrameOffset;
        pbiParams.bIsFirst = true;

		pbiParams.pGameObj = in_pAction->GameObj();

		pbiParams.pTransitionParameters = &Tparameters;
        pbiParams.pContinuousParams = NULL;
        pbiParams.sequenceID = AK_INVALID_SEQUENCE_ID;

		eResult = pNode->Play( pbiParams );

		pNode->Release();
	}
	else
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( in_pAction->UserParam.PlayingID, in_pAction->GameObjID(), in_pAction->UserParam.CustomParam, AkMonitorData::NotificationReason_PlayFailed, HistArray, m_ulElementID, 0 );
		MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedNodeNotAvailablePlay);
		eResult = AK_IDNotFound;
	}
	return eResult;
}

CAkActionPlay* CAkActionPlay::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionPlay*	pActionPlay = AkNew(g_DefaultPoolId,CAkActionPlay(in_eActionType, in_ulID));
	if( pActionPlay )
	{
		if( pActionPlay->Init() != AK_Success )
		{
			pActionPlay->Release();
			pActionPlay = NULL;
		}
	}

	return pActionPlay;
}

void CAkActionPlay::TransitionTime(const AkTimeMs in_lTransitionTime, const AkTimeMs in_RangeMin/*=0*/, const AkTimeMs in_RangeMax /*=0*/)
{
	RandomizerModifier::SetModValue( m_TransitionTime, in_lTransitionTime, in_RangeMin, in_RangeMax );
}

void CAkActionPlay::GetHistArray( AkCntrHistArray& out_rHistArray )
{
	//we don't have any so we give away a clean copy
	out_rHistArray.Init();
}

AKRESULT CAkActionPlay::SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkTimeMs TTime = READBANKDATA( AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMin = READBANKDATA( AkTimeMs, io_rpData, io_rulDataSize );

	AkTimeMs TTimeMax = READBANKDATA( AkTimeMs, io_rpData, io_rulDataSize );

	AkUInt8 ucFadeCurveType = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	m_eFadeCurve = (AkCurveInterpolation)ucFadeCurveType;
	TransitionTime( TTime, TTimeMin, TTimeMax );

	eResult = SetActionSpecificParams(io_rpData, io_rulDataSize);

	if(eResult == AK_Success)
	{
		eResult = SetExceptParams( io_rpData, io_rulDataSize );
	}

	SetFileID( READBANKDATA( AkFileID, io_rpData, io_rulDataSize ) );

	return eResult;
}
