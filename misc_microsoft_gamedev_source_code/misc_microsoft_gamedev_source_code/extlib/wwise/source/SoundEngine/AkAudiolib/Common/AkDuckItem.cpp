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
// AkDuckItem.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkDuckItem.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkBus.h"
#include "AkMonitor.h"

AKRESULT CAkDuckItem::Term()
{
	AKASSERT(g_pTransitionManager);
	if(m_pvVolumeTransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvVolumeTransition);
		m_pvVolumeTransition = NULL;
	}
	return AK_Success;
}


AKRESULT CAkDuckItem::Init(CAkBus* in_pBusNode)
{
	AKASSERT(in_pBusNode);
	m_pvVolumeTransition = NULL;
	m_pBusNode = in_pBusNode;
	m_EffectiveVolumeOffset = 0;
	return AK_Success;
}

void CAkDuckItem::TransUpdateValue(TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated)
{
	// compute the value to be notified
	AkReal32 fNotified = in_unionValue.fValue - m_EffectiveVolumeOffset;

	// set the new value
	m_EffectiveVolumeOffset = in_unionValue.fValue;

	AKASSERT((in_eTargetType & AkTypeMask) == AkTypeFloat);
	if(in_bIsTerminated)
	{
		m_pBusNode->CheckDuck();
		m_pvVolumeTransition = NULL;
	}

	m_pBusNode->VolumeNotification(fNotified);
}
