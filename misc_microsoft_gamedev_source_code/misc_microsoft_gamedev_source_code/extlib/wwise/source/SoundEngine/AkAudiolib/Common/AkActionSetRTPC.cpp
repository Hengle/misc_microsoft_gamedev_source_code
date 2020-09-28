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
// AkActionSetRTPC.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkActionSetRTPC.h"
#include "AkRegisteredObj.h"
#include "AkRTPCMgr.h"
#include "AkBankFloatConversion.h"

#include "AkAudioMgr.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionSetRTPC::CAkActionSetRTPC(AkActionType in_eActionType, AkUniqueID in_ulID) 
: CAkAction( in_eActionType, in_ulID )
,m_RTPC_ID( AK_INVALID_RTPC_ID )
,m_fRTPCValue( 0.f )
{
}

CAkActionSetRTPC::~CAkActionSetRTPC()
{
}

AKRESULT CAkActionSetRTPC::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pRTPCMgr);
	return g_pRTPCMgr->SetRTPCInternal( m_RTPC_ID, m_fRTPCValue, in_pAction->GameObj() );
}

CAkActionSetRTPC* CAkActionSetRTPC::Create( AkActionType in_eActionType, AkUniqueID in_ulID )
{
	CAkActionSetRTPC* pActionSetRTPC = AkNew( g_DefaultPoolId, CAkActionSetRTPC( in_eActionType, in_ulID ) );
	if( pActionSetRTPC )
	{
		if( pActionSetRTPC->Init() != AK_Success )
		{
			pActionSetRTPC->Release();
			pActionSetRTPC = NULL;
		}
	}

	return pActionSetRTPC;
}

void CAkActionSetRTPC::SetRTPCGroup( const AkRtpcID in_RTPCGroupID )
{
	m_RTPC_ID = in_RTPCGroupID;
}

void CAkActionSetRTPC::SetRTPCValue( const AkReal32 in_fRTPCValue )
{
	m_fRTPCValue = in_fRTPCValue;
}

AKRESULT CAkActionSetRTPC::SetActionParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	m_RTPC_ID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	m_fRTPCValue = READBANKDATA(AkReal32, io_rpData, io_rulDataSize);

	return AK_Success;
}
