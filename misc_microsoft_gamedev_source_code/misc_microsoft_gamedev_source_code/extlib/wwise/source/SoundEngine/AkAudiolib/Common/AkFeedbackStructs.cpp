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

#include "stdafx.h"
#include "AkSIMDSpeakerVolumes.h"
#include "AkFeedbackStructs.h"
#ifndef RVL_OS
#include "../SoftwarePipeline/AkSrcLpFilter.h"
#endif

AkFeedbackParams* AkFeedbackParams::Create(AkUInt16 in_iPlayers, AkUInt16 in_iChannels, AkPositioningType in_ePos)
{
	//Only Ak3DGameDef needs to have one volume set per player.  All others need only one.
	if (in_ePos != Ak3DGameDef)
		in_iPlayers = 1;

	//- Times 2 because we need next and previous volumes.
	//- Minus one because there is already one in the structure size.
	AkUInt16 uVolumeCount = in_iPlayers * in_iChannels * 2 - 1;

	//Also, the member m_Volumes must be aligned to 16 bytes boundaries.	
	AkUInt16 uSize = sizeof(AkFeedbackParams) + uVolumeCount * sizeof(AkSIMDSpeakerVolumes);
	AkFeedbackParams *pNew = (AkFeedbackParams*)AK::MemoryMgr::Malign( g_DefaultPoolId, uSize, 16 );
	if (pNew == NULL)
		return NULL;
		
	new (pNew)AkFeedbackParams(in_iPlayers, in_iChannels);

	return pNew;
}

void AkFeedbackParams::Destroy()
{
	this->~AkFeedbackParams();
 	AkFree(g_DefaultPoolId, this);
}

AkFeedbackParams::AkFeedbackParams(AkUInt16 in_iPlayers, AkUInt16 in_iChannels)
{
	m_pOutput = NULL;
	m_NewVolume = 0;
	m_AudioBusVolume = 0;
	m_LPF = 0;
	m_MotionBusPitch = 0;
#ifndef RVL_OS	
	m_pLPFilter = NULL;
#endif	
	m_usCompanyID = 0;
	m_usPluginID = 0;
	for(AkUInt8 i = 0; i < AK_MAX_PLAYERS; i++)
		m_fNextAttenuation[i] = 1.0f;
	m_fNextHierarchyVol = 1.0f;
	m_uPlayerCount = in_iPlayers;
	m_uChannels = in_iChannels;
	m_bFirst = true;
}

AkFeedbackParams::~AkFeedbackParams()
{
#ifndef RVL_OS
	if (m_pLPFilter)
	{
		m_pLPFilter->Term();
		AkDelete2 (g_DefaultPoolId, CAkSrcLpFilter, m_pLPFilter);
		m_pLPFilter = NULL;
	}
#endif
}

bool AkFeedbackParams::CreateLowPassFilter(AkChannelMask in_unChannelMask)
{
	AKASSERT(in_unChannelMask != 0);
	bool bSuccess = false;
#ifndef RVL_OS
#ifdef AK_PS3
	m_pLPFilter = (CAkSrcLpFilter*)AK::MemoryMgr::Malign(g_DefaultPoolId, sizeof(CAkSrcLpFilter), 16);
	new (m_pLPFilter)CAkSrcLpFilter();
#else
	AkNew2( m_pLPFilter, g_DefaultPoolId, CAkSrcLpFilter, CAkSrcLpFilter() );
#endif
	if( m_pLPFilter )
	{
		m_pLPFilter->Init( in_unChannelMask, true );
		bSuccess = true;
	}
#endif	
	return bSuccess;
}


void AkFeedbackParams::StampOldVolumes()
{
	AkUInt16 uTableSize = m_uChannels * m_uPlayerCount;
	memcpy(m_Volumes, &m_Volumes[uTableSize], uTableSize * sizeof(AkSIMDSpeakerVolumes));
}
