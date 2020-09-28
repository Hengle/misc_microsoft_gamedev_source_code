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
// AkActionDuck.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkActionDuck.h"
#include "AkAudiolibIndex.h"
#include "AkAudioMgr.h"
#include "AkBus.h"

extern AkMemPoolId g_DefaultPoolId;

CAkActionDuck::CAkActionDuck(AkActionType in_eActionType, AkUniqueID in_ulID)
: CAkAction(in_eActionType, in_ulID)
{
}

CAkActionDuck::~CAkActionDuck()
{
}

AKRESULT CAkActionDuck::Execute( AkPendingAction * in_pAction )
{
	AKASSERT(g_pIndex);

	CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(m_ulElementID));
	if(pBus)
	{
		pBus->DuckNotif();
		pBus->Release();
	}

	return AK_Success;
}

CAkActionDuck* CAkActionDuck::Create(AkActionType in_eActionType, AkUniqueID in_ulID)
{
	CAkActionDuck*	pActionDuck = AkNew(g_DefaultPoolId,CAkActionDuck(in_eActionType, in_ulID));
	if( pActionDuck )
	{
		if( pActionDuck->Init() != AK_Success )
		{
			pActionDuck->Release();
			pActionDuck = NULL;
		}
	}

	return pActionDuck;
}
