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
// AkBusCtx.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBusCtx.h"
#include "AkBus.h"
#include "AkRTPCMgr.h"
#include "AkEnvironmentsMgr.h"

using namespace AK;

// Sound parameters.
AkUniqueID CAkBusCtx::ID()
{
	if( m_pBus )
	{
		return m_pBus->ID();
	}
	else
	{
		return AK_INVALID_UNIQUE_ID;
	}
}

AkVolumeValue CAkBusCtx::GetVolume()
{
	if( m_pBus )
	{
		return m_pBus->GetBusEffectiveVolume();
	}
	else
	{
		return 0;
	}
}

AkVolumeValue CAkBusCtx::GetLfe()
{
	if( m_pBus )
	{
		return m_pBus->GetBusEffectiveLfe();
	}
	else
	{
		return 0;
	}
}

bool CAkBusCtx::IsMasterBus( )
{
	if ( m_pBus )
		return m_pBus->IsMasterBus();
	else
		return false;
}

// Effects access.
void CAkBusCtx::GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo )
{
	if( m_pBus )
	{
		m_pBus->GetFX( in_uFXIndex, out_rFXInfo );
	}
	else
	{
		out_rFXInfo.EffectTypeID = AK_INVALID_PLUGINID;
		out_rFXInfo.pParam = NULL;
		out_rFXInfo.FxReferenceID = AK_INVALID_UNIQUE_ID;
		out_rFXInfo.bIsBypassed = false;
	}
}

bool CAkBusCtx::GetBypassAllFX()
{
	if ( m_pBus )
		return m_pBus->GetBypassAllFX();

	return false;
}

bool CAkBusCtx::HasEffect()
{
	if( m_pBus )
	{
		return m_pBus->HasEffect();
	}
	else
	{
		return false;
	}
}

bool CAkBusCtx::IsEnvironmental()
{
	if( !m_pBus )
		return false;

	AkFXDesc fxDesc;
	m_pBus->GetFX( 0, fxDesc ); // AK_PLUGINID_ENVIRONMENTAL is set on fx slot 0

	return fxDesc.EffectTypeID == AK_PLUGINID_ENVIRONMENTAL;
}


// Effects access.
AKRESULT CAkBusCtx::SubscribeRTPC( IAkRTPCSubscriber* in_prtpcSubs, AkPluginID in_fxID )
{
	AKRESULT l_eResult = AK_Success;

	if( m_pBus && in_prtpcSubs )
	{
		AkRTPCFXSubscriptionList * l_plistFxRTPC = m_pBus->GetFxRTPCSubscriptionList();

		if( l_plistFxRTPC != NULL )
		{
			for( AkRTPCFXSubscriptionList::Iterator iter = l_plistFxRTPC->Begin(); iter != l_plistFxRTPC->End(); ++iter )
			{
				AkRTPCFXSubscription& l_rFXRTPC = *iter;

				if( in_fxID == l_rFXRTPC.FXID )
				{
					l_eResult = g_pRTPCMgr->SubscribeRTPC( in_prtpcSubs,
														l_rFXRTPC.RTPCID,
														l_rFXRTPC.ParamID,
														l_rFXRTPC.RTPCCurveID,
														l_rFXRTPC.ConversionTable.m_eScaling,
														l_rFXRTPC.ConversionTable.m_pArrayGraphPoints,
														l_rFXRTPC.ConversionTable.m_ulArraySize,
														NULL,
														CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
														);
					AKASSERT( l_eResult == AK_Success );
				}
			}
		}
	}
	else
	{
		l_eResult = AK_Fail;
	}
	return l_eResult;
}

AKRESULT CAkBusCtx::SubscribeRTPCforEnv( IAkRTPCSubscriber* in_prtpcSubs, AkEnvID in_envID )
{
	AKRESULT l_eResult = AK_Success;

	if( m_pBus )
	{
		AkRTPCEnvSubscriptionList * l_plistEnvRTPC = g_pEnvironmentMgr->GetEnvRTPCSubscriptionList();

		if( l_plistEnvRTPC != NULL )
		{
			for( AkRTPCEnvSubscriptionList::Iterator iter = l_plistEnvRTPC->Begin(); iter != l_plistEnvRTPC->End(); ++iter )
			{
				AkRTPCEnvSubscription& l_rEnvRTPC = *iter;

				if( in_prtpcSubs && in_envID == l_rEnvRTPC.EnvID )
				{
					l_eResult = g_pRTPCMgr->SubscribeRTPC( in_prtpcSubs,
														l_rEnvRTPC.RTPCID,
														l_rEnvRTPC.ParamID,
														l_rEnvRTPC.RTPCCurveID,
														l_rEnvRTPC.ConversionTable.m_eScaling,
														l_rEnvRTPC.ConversionTable.m_pArrayGraphPoints,
														l_rEnvRTPC.ConversionTable.m_ulArraySize,
														NULL,
														CAkRTPCMgr::SubscriberType_IAkRTPCSubscriber
														);
					AKASSERT( l_eResult == AK_Success );
				}
			}
		}
	}
	else
	{
		l_eResult = AK_Fail;
	}
	return l_eResult;
}

AKRESULT CAkBusCtx::UnsubscribeRTPC( IAkRTPCSubscriber* in_pRtpcSubs )
{
	return g_pRTPCMgr->UnSubscribeRTPC( in_pRtpcSubs );
}
