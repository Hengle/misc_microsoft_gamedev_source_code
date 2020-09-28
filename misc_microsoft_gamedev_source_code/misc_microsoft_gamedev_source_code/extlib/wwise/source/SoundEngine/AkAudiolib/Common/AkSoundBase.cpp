/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkSoundBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMonitor.h"
#include "AkSoundBase.h"
#include "AkModifiers.h"
#include "AkURenderer.h"
#include "AkAudioLibIndex.h"

CAkSoundBase::CAkSoundBase( 
    AkUniqueID in_ulID
    )
:CAkParameterNode(in_ulID)
,m_Loop( AkLoopVal_NotLooping )
{
	m_listPBI.Init();
}

CAkSoundBase::~CAkSoundBase()
{
	m_listPBI.Term();
}

AKRESULT CAkSoundBase::PlayToEnd( CAkRegisteredObj * in_pGameObj, AkUniqueID in_nodeID, AkPlayingID in_PlayingID )
{
	CAkAudioNode * pTarget = NULL;
	
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
	{
		CAkPBI * l_pPBI = *iter;
		if( ( !in_pGameObj || l_pPBI->GetGameObjectPtr() == in_pGameObj ) &&
			( in_PlayingID == AK_INVALID_PLAYING_ID || in_PlayingID == l_pPBI->GetPlayingID() ) )
		{
			if ( pTarget == NULL )
			{
				pTarget = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_nodeID );
				if ( !pTarget ) 
					return AK_Fail;
			}

			l_pPBI->PlayToEnd( pTarget );
		}
	}
	
	if ( pTarget )
		pTarget->Release();

	return AK_Success;
}

void CAkSoundBase::ParamNotification( NotifParams& in_rParams )
{
	CAkPBI* l_pPBI    = NULL;

	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
	{
		l_pPBI = *iter;
		AKASSERT( l_pPBI != NULL );

		if( in_rParams.pExceptObjects == NULL || in_rParams.pGameObj != NULL )
		{
			if(	(in_rParams.pGameObj == NULL) 
				|| l_pPBI->GetGameObjectPtr() == in_rParams.pGameObj )
			{
				// Update Behavioural side.
				l_pPBI->ParamNotification( in_rParams );
			}
		}
		else
		{
			GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_rParams.pExceptObjects );
			bool l_bIsException = false;
			for( GameObjExceptArray::Iterator iter = l_pExceptArray->Begin(); iter != l_pExceptArray->End(); ++iter )
			{
				if( *(iter.pItem) == l_pPBI->GetGameObjectPtr() )
				{
					l_bIsException = true;
					break;
				}
			}
			if( !l_bIsException )
			{
				l_pPBI->ParamNotification( in_rParams );
			}
		}
	}
}

void CAkSoundBase::MuteNotification( AkUInt8 in_cMuteLevel, AkMutedMapItem& in_rMutedItem, bool in_bIsFromBus /*= false*/ )
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		(*iter)->MuteNotification( in_cMuteLevel, in_rMutedItem, false );
	}
}

void CAkSoundBase::MuteNotification( AkUInt8 in_cMuteLevel, CAkRegisteredObj * in_pGameObj, AkMutedMapItem& in_rMutedItem, bool in_bPrioritizeGameObjectSpecificItems /* = false */ )
{
	const bool bForceNotify = !in_pGameObj || ( in_bPrioritizeGameObjectSpecificItems && ! in_pGameObj );

	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		CAkPBI * l_pPBI = *iter;

		if( bForceNotify || l_pPBI->GetGameObjectPtr() == in_pGameObj )
		{
			// Update Behavioural side.
			l_pPBI->MuteNotification( in_cMuteLevel, in_rMutedItem, in_bPrioritizeGameObjectSpecificItems );
		}
	}

}

// Notify the children PBI that a change int the Positioning parameters occured from RTPC
void CAkSoundBase::PositioningChangeNotification( AkReal32 in_RTPCValue, AkRTPC_ParameterID in_ParameterID, CAkRegisteredObj * in_pGameObj, void*	in_pExceptArray )
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		CAkPBI * l_pPBI = *iter;

		if( in_pExceptArray == NULL || in_pGameObj != NULL )
		{
			if(	( in_pGameObj == NULL ) 
				|| l_pPBI->GetGameObjectPtr() == in_pGameObj )
			{
				// Update Behavioural side.
				l_pPBI->PositioningChangeNotification( in_RTPCValue, in_ParameterID );
			}
		}
		else
		{
			GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_pExceptArray );
			bool l_bIsException = false;
			for( GameObjExceptArray::Iterator iter = l_pExceptArray->Begin(); iter != l_pExceptArray->End(); ++iter )
			{
				if( *(iter.pItem) == l_pPBI->GetGameObjectPtr() )
				{
					l_bIsException = true;
					break;
				}
			}
			if( !l_bIsException )
			{
				l_pPBI->PositioningChangeNotification( in_RTPCValue, in_ParameterID );
			}
		}
	}
}

void CAkSoundBase::RecalcNotification()
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
	{
		(*iter)->RecalcNotification();
	}
}

void CAkSoundBase::NotifyBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask,
	CAkRegisteredObj * in_pGameObj,
	void* in_pExceptArray /* = NULL */ )
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		CAkPBI* l_pPBI = *iter;

		if( in_pExceptArray == NULL || in_pGameObj != NULL )
		{
			if(	( in_pGameObj == NULL ) 
				|| l_pPBI->GetGameObjectPtr() == in_pGameObj )
			{
				// Update Behavioural side.
				l_pPBI->NotifyBypass( in_bitsFXBypass, in_uTargetMask );
			}
		}
		else
		{
			GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_pExceptArray );
			bool l_bIsException = false;
			for( GameObjExceptArray::Iterator iter = l_pExceptArray->Begin(); iter != l_pExceptArray->End(); ++iter )
			{
				if( *(iter.pItem) == l_pPBI->GetGameObjectPtr() )
				{
					l_bIsException = true;
					break;
				}
			}
			if( !l_bIsException )
			{
				l_pPBI->NotifyBypass( in_bitsFXBypass, in_uTargetMask );
			}
		}
	}
}

void CAkSoundBase::NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		(*iter)->NotifUnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );
	}
}

void CAkSoundBase::UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem )
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		(*iter)->UpdateRTPC( in_rSubsItem );
	}
}

#ifndef AK_OPTIMIZED
void CAkSoundBase::UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pParamsBlock,
		AkUInt32			in_ulParamBlockSize
		)
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
    {
		(*iter)->UpdateFxParam( in_FXID, in_uFXIndex, in_ulParamID, in_pParamsBlock, in_ulParamBlockSize );
	}
}

void CAkSoundBase::InvalidatePaths()
{
	for( AkListLightCtxs::Iterator iter = m_listPBI.Begin(); iter != m_listPBI.End(); ++iter )
	{
		(*iter)->InvalidatePaths();
	}
}

#endif

AkRTPCFXSubscriptionList* CAkSoundBase::GetSourceRTPCSubscriptionList()
{
	return m_pFXChunk ? &( m_pFXChunk->listFXRTPCSubscriptions ) : NULL;
}

void CAkSoundBase::Loop( bool  in_bIsLoopEnabled,
							    bool  in_bIsInfinite,
							    AkInt16 in_ulLoopCount,
							    AkInt16 in_lCountModMin,
							    AkInt16 in_lCountModMax
							   )
{
	if(in_bIsLoopEnabled)
	{
		if(in_bIsInfinite)
		{
			m_Loop = AkLoopVal_Infinite;
			m_LoopMod.m_min = 0;
			m_LoopMod.m_max = 0;
		}
		else
		{
			m_Loop = in_ulLoopCount;
			m_LoopMod.m_min = in_lCountModMin;
			m_LoopMod.m_max = in_lCountModMax;
		}
	}
	else
	{
		m_Loop = AkLoopVal_NotLooping;
		m_LoopMod.m_min = 0;
		m_LoopMod.m_max = 0;
	}
}

AkInt16 CAkSoundBase::Loop()
{
	return m_Loop + RandomizerModifier::GetMod( m_LoopMod );
}

void CAkSoundBase::MonitorNotif(AkMonitorData::NotificationReason in_eNotifReason, AkGameObjectID in_GameObjID, UserParams& in_rUserParams, PlayHistory& in_rPlayHistory)
{
	MONITOR_OBJECTNOTIF( in_rUserParams.PlayingID, in_GameObjID, in_rUserParams.CustomParam, in_eNotifReason, in_rPlayHistory.HistArray, ID(), 0 );
}
