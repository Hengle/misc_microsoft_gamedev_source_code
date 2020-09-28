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
// AkBus.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBus.h"
#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include "AkActionDuck.h"
#include "AkTransitionManager.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkBankFloatConversion.h"
#include "AkRegistryMgr.h"
#include "AkDuckItem.h"
#include "Ak3DParams.h"
#include "AkSIS.h"
#include "AkURenderer.h"
#include "AkLEngine.h"
#include "AkRTPCMgr.h"
#include "AkBanks.h"
#include "AkEnvironmentsMgr.h"
#include "AkBankMgr.h"

#ifdef XBOX360
#include "Xmp.h"
#endif

#define AK_MIN_DUCK_LIST_SIZE 0
#define AK_MAX_DUCK_LIST_SIZE 100

extern AkMemPoolId g_DefaultPoolId;

CAkBusCtx	g_MasterBusCtx;
CAkBus *	g_pMasterBus = NULL;

CAkBus::CAkBus(AkUniqueID in_ulID)
:CAkActiveParent<CAkParameterNodeBase>(in_ulID)
,m_RecoveryTime(0)
,m_eDuckingState(DuckState_OFF)
,m_iPlayCountValid(0)
#ifdef XBOX360
,m_bIsXMPBus(false)
,m_bIsXMPMuted(false)
#endif
{
}

CAkBus::~CAkBus()
{

#ifdef XBOX360
	XMP_UnsetAsXMPBus();
#endif

	for( AkDuckedVolumeList::Iterator iter = m_DuckedVolumeList.Begin(); iter != m_DuckedVolumeList.End(); ++iter )
	{
		(*iter).item.Term(); //Terminating the CAkDuckItem in the list
	}
	m_DuckedVolumeList.Term();
	m_ToDuckList.Term();

	if ( this == g_pMasterBus )
	{
		 g_pMasterBus = NULL;
		 g_MasterBusCtx.SetBus( NULL );
		 g_pBankManager->SetIsFirstBusLoaded( false );
		 CAkBankMgr::SignalLastBankUnloaded();
	}
}

AKRESULT CAkBus::Init()
{
	AKRESULT eResult = CAkActiveParent<CAkParameterNodeBase>::Init();
	if( eResult == AK_Success )
	{
		eResult = m_ToDuckList.Init( AK_MIN_DUCK_LIST_SIZE, AK_MAX_DUCK_LIST_SIZE, g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_DuckedVolumeList.Init( AK_MIN_DUCK_LIST_SIZE, AK_MAX_DUCK_LIST_SIZE, g_DefaultPoolId );
	}
	return eResult;
}

CAkBus* CAkBus::Create( AkUniqueID in_ulID )
{
	CAkBus* pBus = AkNew( g_DefaultPoolId, CAkBus( in_ulID ) );
	if( pBus )
	{
		if( pBus->Init() != AK_Success )
		{
			pBus->Release();
			pBus = NULL;
		}
		else if ( g_pMasterBus == NULL )
		{
			g_pMasterBus = pBus;
			g_MasterBusCtx.SetBus( pBus );
		}
	}

	return pBus;
}

AkNodeCategory CAkBus::NodeCategory()
{
	return AkNodeCategory_Bus;
}

AKRESULT CAkBus::Play( AkPBIParams& in_rPBIParams )
{
	AKASSERT(!"Play() Shouldn't be called directly on a BUS"); 
	return AK_NotImplemented;
}

AKRESULT CAkBus::ExecuteAction( ActionParams& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( IsActiveOrPlaying() )
	{
		if( in_rAction.bIsMasterCall )
		{
			bool bPause = false;
			if( in_rAction.eType == ActionParamType_Resume )
			{
				bPause = true;
			}
			PauseTransitions( bPause );
		}
		in_rAction.bIsFromBus = true;
		for( AkMapChildID::Iterator iter = m_mapChildId.Begin(); iter != m_mapChildId.End(); ++iter )
		{
			eResult = (*iter).item->ExecuteAction( in_rAction );
			if( eResult != AK_Success )
			{
				break;
			}
		}
	}
	return eResult;
}

AKRESULT CAkBus::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( in_rAction.pGameObj == NULL )
	{
		bool bPause = false;
		if( in_rAction.eType == ActionParamType_Resume )
		{
			bPause = true;
		}
		PauseTransitions( bPause );
	}

	in_rAction.bIsFromBus = true;

	for( AkMapChildID::Iterator iter = m_mapChildId.Begin(); iter != m_mapChildId.End(); ++iter )
	{
		if(!IsException( (*iter).key, *(in_rAction.pExeceptionList) ) )
		{
			eResult = (*iter).item->ExecuteActionExcept( in_rAction );
			if( eResult != AK_Success )
			{
				break;
			}
		}
	}
	return eResult;
}

AKRESULT CAkBus::GetAudioParameters(AkSoundParams &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_pGameObj, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/)
{
	AKRESULT eResult = AK_Success;

	if(HasEffect())
	{
		in_ulParamSelect = (in_ulParamSelect &= ~PT_Volume);
		in_ulParamSelect = (in_ulParamSelect &= ~PT_LFE);
	}
	else
	{
		AkVolumeValue DuckedVolume = GetDuckedVolume();
		io_Parameters.Volume += DuckedVolume;
		io_Parameters.LFE += DuckedVolume;
	}

	AkUInt32 ulParamSelect = in_ulParamSelect;
	if( m_bUseState && m_pStateTransitionInfo )
	{
		CAkState* pState = GetState();
		if( pState != NULL )
		{
			if(in_ulParamSelect & PT_Volume)
			{
				if(pState->VolumeMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect &= ~PT_Volume;
				}
				io_Parameters.Volume += m_pStateTransitionInfo->m_Volume;
			}
			if(in_ulParamSelect & PT_Pitch)
			{
				if(pState->PitchMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect &= ~PT_Pitch;
				}
				io_Parameters.Pitch += m_pStateTransitionInfo->m_Pitch;
			}
			if(in_ulParamSelect & PT_LFE)
			{
				if(pState->LFEVolumeMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect = (in_ulParamSelect & ~PT_LFE);
				}
				io_Parameters.LFE += m_pStateTransitionInfo->m_Lfe;
			}
			if(in_ulParamSelect & PT_LPF)
			{
				if(pState->LPFMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect = (in_ulParamSelect & ~PT_LPF);
				}
				io_Parameters.LPF += m_pStateTransitionInfo->m_LPF;
			}
		}
		// we don't have any state
		else
		{
			if(in_ulParamSelect & PT_Volume)
			{
				io_Parameters.Volume += m_pStateTransitionInfo->m_Volume;
			}
			if(in_ulParamSelect & PT_Pitch)
			{
				io_Parameters.Pitch += m_pStateTransitionInfo->m_Pitch;
			}
			if(in_ulParamSelect & PT_LFE)
			{
				io_Parameters.LFE += m_pStateTransitionInfo->m_Lfe;
			}
			if(in_ulParamSelect & PT_LPF)
			{
				io_Parameters.LPF += m_pStateTransitionInfo->m_LPF;
			}
		}
	}

	if(in_ulParamSelect & PT_Volume)
	{
		io_Parameters.Volume += m_VolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Volume ) )
		{
			io_Parameters.Volume += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Volume, NULL );
		}
	}
	if(in_ulParamSelect & PT_Pitch)
	{
		io_Parameters.Pitch += m_PitchMain;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Pitch ) )
		{
			io_Parameters.Pitch += (AkPitchValue) g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Pitch, NULL );
		}
	}
	if(in_ulParamSelect & PT_LFE)
	{
		io_Parameters.LFE += m_LFEVolumeMain.GetValue();
		if(m_RTPCBitArrayMax32.IsSet( RTPC_LFE ))
		{
			io_Parameters.LFE += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_LFE, NULL );
		}
	}
	if(in_ulParamSelect & PT_LPF)
	{
		io_Parameters.LPF += m_LPFMain;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_LPF ) )
		{
			io_Parameters.LPF += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_LPF, NULL );
		}
	}
	if(m_pGlobalSIS)
	{
		io_Parameters.Volume += m_pGlobalSIS->m_EffectiveVolumeOffset;
		io_Parameters.LFE += m_pGlobalSIS->m_EffectiveLFEOffset;
		io_Parameters.LPF += m_pGlobalSIS->m_EffectiveLPFOffset;
		io_Parameters.Pitch += m_pGlobalSIS->m_EffectivePitchOffset;
		if( m_pGlobalSIS->m_cMuteLevel != UNMUTED_LVL )
		{
			AkMutedMapItem item;
            item.m_bIsPersistent = false;
			item.m_bIsGlobal = true;
			item.m_Identifier = this;
			io_rMutedMap.Set( item, m_pGlobalSIS->m_cMuteLevel );
		}
	}

#ifdef XBOX360
	if(m_bIsXMPMuted)
	{
		AkMutedMapItem item;
        item.m_bIsPersistent = false;
		item.m_bIsGlobal = true;
		item.m_Identifier = this;
		io_rMutedMap.Set( item, MUTED_LVL );//This override possible other mute level
	}
#endif

	if(m_pBusOutputNode != NULL)
	{
		static_cast<CAkParameterNodeBase*>(m_pBusOutputNode)->GetAudioParameters( io_Parameters, ulParamSelect, io_rMutedMap, in_pGameObj, in_bIncludeRange, io_Ranges );
	}
	return eResult;
}

AkVolumeValue CAkBus::GetBusEffectiveVolume()
{
	AkVolumeValue l_Volume = AK_MAXIMUM_VOLUME_LEVEL;
	bool l_bVolumeIndependent = false;
	if( m_bUseState && m_pStateTransitionInfo )
	{
		CAkState* pState = GetState();
		if(pState != NULL)
		{
			if(pState->VolumeMeaning() == AkValueMeaning_Independent)
			{
				l_bVolumeIndependent = true;
			}
		}
		l_Volume += m_pStateTransitionInfo->m_Volume;
	}

	if( !l_bVolumeIndependent )
	{
		l_Volume += m_VolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Volume ) )
		{
			l_Volume += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Volume, NULL );
		}
	}
	if(m_pGlobalSIS)
	{
		l_Volume += m_pGlobalSIS->m_EffectiveVolumeOffset;
	}

	l_Volume += GetDuckedVolume();

	if( m_pBusOutputNode != NULL )
	{
		l_Volume += static_cast<CAkBus*>(m_pBusOutputNode)->GetBusEffectiveVolume();
	}

	return l_Volume;
}

AkVolumeValue CAkBus::GetBusEffectiveLfe()
{
	AkVolumeValue l_Lfe = AK_MAXIMUM_VOLUME_LEVEL;
	bool l_bVolumeIndependent = false;
	if( m_bUseState && m_pStateTransitionInfo )
	{
		CAkState* pState = GetState();
		if(pState != NULL)
		{
			if(pState->LFEVolumeMeaning() == AkValueMeaning_Independent)
			{
				l_bVolumeIndependent = true;
			}
		}
		l_Lfe += m_pStateTransitionInfo->m_Lfe;
	}

	if( !l_bVolumeIndependent )
	{
		l_Lfe += m_LFEVolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_LFE ) )
		{
			l_Lfe += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_LFE, NULL );
		}
	}
	if(m_pGlobalSIS)
	{
		l_Lfe += m_pGlobalSIS->m_EffectiveLFEOffset;
	}

	l_Lfe += GetDuckedVolume();

	if( m_pBusOutputNode != NULL )
	{
		l_Lfe += static_cast<CAkBus*>(m_pBusOutputNode)->GetBusEffectiveLfe();
	}

	return l_Lfe;
}


void CAkBus::Volume(AkVolumeValue in_Volume)
{
	if( DoesChangeMustBeNotified( PT_Volume ) )
	{
		VolumeNotification( in_Volume - m_VolumeMain.GetValue() );
	}
	m_VolumeMain.SetValue( in_Volume );
}

void CAkBus::Pitch(AkPitchValue in_Pitch)
{
	if( DoesChangeMustBeNotified( PT_Pitch ) )
	{
		PitchNotification( in_Pitch - m_PitchMain );
	}
	m_PitchMain = (AkInt16)in_Pitch;
}

void CAkBus::LFEVolume( AkVolumeValue in_LFEVolume )
{
	if( DoesChangeMustBeNotified( PT_LFE ) )
	{
		LFENotification( in_LFEVolume - m_LFEVolumeMain.GetValue() );
	}
	m_LFEVolumeMain.SetValue( in_LFEVolume );
}

void CAkBus::LPF( AkLPFType in_LPF )
{
	if( DoesChangeMustBeNotified( PT_LPF ) )
	{
		LPFNotification( in_LPF - m_LPFMain );
	}
	m_LPFMain = (AkUInt8)in_LPF;
}

void CAkBus::ParamNotification( NotifParams& in_rParams )
{
	AKASSERT( in_rParams.pGameObj == NULL );

	// Note: master bus and bus volumes are applied lower in the hierarchy when the is no effect, 
	// otherwise they are applied at the proper level to avoid having pre-effect volumes
	in_rParams.bIsFromBus = true;
	if( (in_rParams.eType == NotifParamType_Volume) && HasEffect() )
	{
		if ( IsMasterBus() )
			CAkLEngine::SetMasterBusVolume( in_rParams.UnionType.volume );
		else
			CAkLEngine::SetBusVolume( ID(), in_rParams.UnionType.volume );
	}
	else if( (in_rParams.eType == NotifParamType_LFE) && HasEffect() )
	{
		if ( IsMasterBus() )
			CAkLEngine::SetMasterBusLFE( in_rParams.UnionType.LFE );
		else
			CAkLEngine::SetBusLFE( ID(), in_rParams.UnionType.LFE );
	}
	else
	{
		AKASSERT( in_rParams.pExceptObjects == NULL );

		// Propagate notification to all children (including volumes if the is no effects)
		for( AkMapChildID::Iterator iter = m_mapChildId.Begin(); iter != m_mapChildId.End(); ++iter )
		{
			// Does not check if playing to allow notification during effect tail.
			(*iter).item->ParamNotification( in_rParams );
		}
	}
}

void CAkBus::MuteNotification(AkUInt8 in_cMuteLevel, AkMutedMapItem& in_rMutedItem, bool in_bIsFromBus /*= false*/)
{
	for( AkMapChildID::Iterator iter = m_mapChildId.Begin(); iter != m_mapChildId.End(); ++iter )
	{
		if( (*iter).item->IsPlaying() )
		{
#ifdef XBOX360
			if( m_bIsXMPMuted && in_rMutedItem.m_Identifier == this )
			{
				in_cMuteLevel = MUTED_LVL;
			}
#endif
			(*iter).item->MuteNotification(in_cMuteLevel, in_rMutedItem, true);
		}
	}
}

void CAkBus::NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_GameObj,
		void*	in_pExceptArray /* = NULL */)
{
	if ( IsMasterBus() )
	{
		CAkLEngine::BypassMasterBusFx( in_bitsFXBypass, in_uTargetMask );
	}
	else
	{
		CAkLEngine::BypassBusFx( ID(), in_bitsFXBypass, in_uTargetMask );
		if( m_pFXChunk && m_pFXChunk->aFX[ 0 ].id == AK_PLUGINID_ENVIRONMENTAL )
		{
			g_pEnvironmentMgr->BypassAllEnv( ( in_bitsFXBypass & ( (1<<0) | (1<<AK_NUM_EFFECTS_BYPASS_ALL_FLAG) ) ) != 0 );
		}
	}
}

void CAkBus::NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
#ifndef AK_OPTIMIZED
	if ( IsMasterBus() )
		CAkURenderer::NotifMasterBusUnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );
	else
		CAkURenderer::NotifBusUnsetRTPC( ID(), in_FXID, in_ParamID, in_RTPCCurveID );
#endif
}

void CAkBus::UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem )
{
#ifndef AK_OPTIMIZED
	if ( IsMasterBus() )
		CAkURenderer::UpdateMasterBusRTPC( in_rSubsItem );
	else
		CAkURenderer::UpdateBusRTPC( ID(), in_rSubsItem );
#endif
}

AKRESULT CAkBus::CanAddChild( CAkAudioNode * in_pAudioNode )
{
	AKASSERT(in_pAudioNode);

	AKRESULT eResult = AK_Success;	
	if(in_pAudioNode->ParentBus() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

AKRESULT CAkBus::AddChild(AkUniqueID in_ulID)
{
	AKASSERT(g_pIndex);
	if(!in_ulID)
	{
		return AK_InvalidID;
	}
	
	CAkAudioNode* pAudioNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_ulID);
	if ( !pAudioNode )
		return AK_IDNotFound;

	AKRESULT eResult = CanAddChild(pAudioNode);
	if(eResult == AK_Success)
	{
		if( !m_mapChildId.Set( in_ulID, pAudioNode ) )
		{
			eResult = AK_Fail;
		}
		else
		{
			pAudioNode->ParentBus(this);
			this->AddRef();
		}
	}

	pAudioNode->Release();

	return eResult;
}

AKRESULT CAkBus::RemoveChild(AkUniqueID in_ulID)
{
	AKASSERT(in_ulID);
	AKRESULT eResult = AK_Success;
	CAkAudioNode** l_ppANPtr = m_mapChildId.Exists(in_ulID);
	if( l_ppANPtr )
	{
		(*l_ppANPtr)->ParentBus(NULL);
		m_mapChildId.Unset(in_ulID);
		this->Release();
	}
	return eResult;
}

///////////////////////////////////////////////////////////////////////////////
//                       DUCKING FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
AKRESULT CAkBus::AddDuck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve
		)
{
	AKASSERT(in_BusID);
	
	AkDuckInfo * pDuckInfo = m_ToDuckList.Set( in_BusID );
	if ( pDuckInfo )
	{
		pDuckInfo->DuckVolume = in_DuckVolume;
		pDuckInfo->FadeCurve = in_eFadeCurve;
		pDuckInfo->FadeInTime = in_FadeInTime;
		pDuckInfo->FadeOutTime = in_FadeOutTime;

		return AK_Success;
	}
	
	return AK_Fail;
}

AKRESULT CAkBus::RemoveDuck(
		AkUniqueID in_BusID
		)
{
	AKASSERT( g_pIndex );

#ifndef AK_OPTIMIZED
	AkDuckInfo* pDuckInfo = m_ToDuckList.Exists( in_BusID );
	if( pDuckInfo )
	{
		CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_BusID ));
		if( pBus )
		{
			// Remove potential duck that may have occured on the Ducked Bus
			pBus->Unduck( ID(), 0, pDuckInfo->FadeCurve );
			pBus->Release();
		}
	}
#endif //AK_OPTIMIZED
	m_ToDuckList.Unset(in_BusID);
	return AK_Success;
}

AKRESULT CAkBus::RemoveAllDuck()
{
	m_ToDuckList.RemoveAll();
	return AK_Success;
}

void CAkBus::SetRecoveryTime(AkUInt32 in_RecoveryTime)
{
	m_RecoveryTime = in_RecoveryTime;
}

AKRESULT CAkBus::Duck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkCurveInterpolation in_eFadeCurve
		)
{
	AKRESULT eResult = AK_Success;
	CAkDuckItem* pDuckItem = m_DuckedVolumeList.Exists(in_BusID);
	if(!pDuckItem)
	{
		pDuckItem = m_DuckedVolumeList.Set(in_BusID);
		if ( pDuckItem )
		{
			pDuckItem->Init( this );
		}
		else
		{
			eResult = AK_Fail;
		}
	}
	
	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Ducked, 0, 0);

	if(eResult == AK_Success)
	{
		StartDuckTransitions(pDuckItem, in_DuckVolume, AkValueMeaning_Offset, in_eFadeCurve, in_FadeOutTime);
	}

	return eResult;
}

AKRESULT CAkBus::Unduck(
		AkUniqueID in_BusID,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve
		)
{
	AKRESULT eResult = AK_Success;
	CAkDuckItem* pDuckItem = m_DuckedVolumeList.Exists(in_BusID);
	if(pDuckItem)
	{
		StartDuckTransitions(pDuckItem, 0, AkValueMeaning_Default, in_eFadeCurve, in_FadeInTime);
		CheckDuck();
	}

	return eResult;
}

AKRESULT CAkBus::PauseDuck(
		AkUniqueID in_BusID
		)
{
	AKRESULT eResult = AK_Success;
	CAkDuckItem* pDuckItem = m_DuckedVolumeList.Exists(in_BusID);
	if(pDuckItem)
	{
		//Setting the transition time to zero with initial value, which will freeze the duck value while waiting
		StartDuckTransitions(pDuckItem, pDuckItem->m_EffectiveVolumeOffset, AkValueMeaning_Independent, AkCurveInterpolation_Linear, 0);
	}

	return eResult;
}

void CAkBus::StartDuckTransitions(CAkDuckItem*		in_pDuckItem,
										AkReal32			in_fTargetValue,
										AkValueMeaning	in_eValueMeaning,
										AkCurveInterpolation		in_eFadeCurve,
										AkTimeMs		in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);

	TransitionParameters	VolumeParams;

	// have we got one running already ?
	if(in_pDuckItem->m_pvVolumeTransition != NULL)
	{
		VolumeParams.uTargetValue.fValue = in_fTargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pDuckItem->m_pvVolumeTransition,
													TransTarget_Volume,
													VolumeParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning);
	}

	// got none running yet ?
	if(in_pDuckItem->m_pvVolumeTransition == NULL)
	{
		VolumeParams.uStartValue.fValue = in_pDuckItem->m_EffectiveVolumeOffset;

		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Offset:
		case AkValueMeaning_Independent:
			VolumeParams.uTargetValue.fValue = in_fTargetValue;
			break;
		case AkValueMeaning_Default:
			VolumeParams.uTargetValue.fValue = 0.0f;
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((VolumeParams.uStartValue.fValue == VolumeParams.uTargetValue.fValue)
			|| (in_lTransitionTime == 0))
		{
			// no need to
			AkReal32 fNotifValue = in_pDuckItem->m_EffectiveVolumeOffset;
			in_pDuckItem->m_EffectiveVolumeOffset = VolumeParams.uTargetValue.fValue;

			VolumeNotification(in_pDuckItem->m_EffectiveVolumeOffset - fNotifValue);

		}
		// start it
		else
		{
			VolumeParams.eTargetType = AkVolumeFloat;
			VolumeParams.eFadeCurve = in_eFadeCurve;
			VolumeParams.lDuration = in_lTransitionTime;
			VolumeParams.bdBs = true;
			VolumeParams.pUser = in_pDuckItem;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pDuckItem->m_pvVolumeTransition = g_pTransitionManager->AddTransitionToList(VolumeParams);
		}
	}
}

bool CAkBus::IncrementPlayCount( 
	AkPriority in_Priority, 
	CAkRegisteredObj * in_GameObj, 
	AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/,
	AkUInt16 in_ui16NumKicked /*= 0*/, 
	bool in_bMaxConsidered /*= false*/ 
	)
{
	bool bCanPlayCurrent = true;

	++m_PlayCount;

	if( !in_bMaxConsidered )
	{
		if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
		{
			++m_iPlayCountValid;
			in_bMaxConsidered = true;
			if( IsMaxNumInstancesActivated() && ( m_iPlayCountValid - in_ui16NumKicked ) > GetMaxNumInstances() )
			{
				CAkAudioNode* pKicked = NULL;
				bCanPlayCurrent = CAkURenderer::Kick( in_Priority, NULL, ID(), m_bKillNewest, pKicked );
				++in_ui16NumKicked;
			}
		}
	}

	if(m_pBusOutputNode)
	{
		bCanPlayCurrent = m_pBusOutputNode->IncrementPlayCount( in_Priority, in_GameObj, AK_ForwardToBusType_ALL, in_ui16NumKicked, in_bMaxConsidered ) && bCanPlayCurrent;
	}

	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		bCanPlayCurrent = m_pFeedbackInfo->m_pFeedbackBus->IncrementPlayCount(in_Priority, in_GameObj, AK_ForwardToBusType_ALL, in_ui16NumKicked, in_bMaxConsidered) && bCanPlayCurrent;
	}

	if(m_PlayCount == 1)
	{
		//Must start Ducking
		m_eDuckingState = DuckState_ON;
		UpdateDuckedBus();
	}

	return bCanPlayCurrent;
}

void CAkBus::DecrementPlayCount( 
	CAkRegisteredObj * in_GameObj, 
	AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/, 
	bool in_bMaxConsidered /*=false*/ 
	)
{
	--m_PlayCount;

	if( !in_bMaxConsidered )
	{
		
		if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
		{
			--m_iPlayCountValid;
			in_bMaxConsidered = true;
		}
	}

	if(m_pBusOutputNode)
	{
		m_pBusOutputNode->DecrementPlayCount( in_GameObj, AK_ForwardToBusType_ALL, in_bMaxConsidered );
	}

	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		m_pFeedbackInfo->m_pFeedbackBus->DecrementPlayCount(in_GameObj, AK_ForwardToBusType_ALL, in_bMaxConsidered);
	}

	AKASSERT( !m_pParentNode ); // Should never happen as actually a bus never has parents, just making sure.

	if(m_PlayCount == 0)
	{
		//Must stop Ducking
		if(m_RecoveryTime)
		{
			m_eDuckingState = DuckState_PENDING;
			UpdateDuckedBus();

			if(!m_ToDuckList.IsEmpty())
			{
				AKRESULT eResult = RequestDuckNotif();
				if(eResult != AK_Success)
				{
					// If we cannot get a notif, then just skip the notif and ask for fade back right now.
					// Better doing it now than never
					m_eDuckingState = DuckState_OFF;
					UpdateDuckedBus();
				}
			}
		}
		else
		{
			m_eDuckingState = DuckState_OFF;
			UpdateDuckedBus();
		}
	}
}

void CAkBus::IncrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	++m_uActivityCount;
	AKASSERT( m_uActivityCount );//wrapped around, this will not cause bad damage, but unrequired notifications car be triggered.

	if( m_pBusOutputNode )
	{
		m_pBusOutputNode->IncrementActivityCount();
	}

	if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
	{
		m_pFeedbackInfo->m_pFeedbackBus->IncrementActivityCount();
	}
}

void CAkBus::DecrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	AKASSERT( m_uActivityCount );// we had more decrement than increment, not normal.
	--m_uActivityCount;

	if( m_pBusOutputNode )
	{
		m_pBusOutputNode->DecrementActivityCount();
	}

	if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
	{
		m_pFeedbackInfo->m_pFeedbackBus->DecrementActivityCount();
	}
}

bool CAkBus::IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest )
{
	bool bRet = in_NodeIDToTest == ID();
	if( !bRet )
	{
		if( ParentBus() != NULL && !m_bIsMaxNumInstOverrideParent )
		{
			bRet = static_cast<CAkBus*>(ParentBus() )->IsInstanceCountCompatible( in_NodeIDToTest );
		}
	}
	return bRet;
}

bool CAkBus::IsOrIsChildOf( AkUniqueID in_NodeIDToTest )
{
	CAkBus* pBus = this;
	while( pBus )
	{
		if( in_NodeIDToTest == pBus->ID() )
			return true;
		pBus = static_cast<CAkBus*>( pBus->ParentBus() );
	}
	return false;
}

void CAkBus::DuckNotif()
{
	if(m_eDuckingState == DuckState_PENDING)
	{
		m_eDuckingState = DuckState_OFF;
		UpdateDuckedBus();
	}
}

void CAkBus::UpdateDuckedBus()
{
	for( AkToDuckList::Iterator iter = m_ToDuckList.Begin(); iter != m_ToDuckList.End(); ++iter )
	{
		MapStruct<AkUniqueID, AkDuckInfo>&  rItem = *iter;
		CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef(rItem.key));
		if(pBus)
		{
			switch(m_eDuckingState)
			{
			case DuckState_OFF:
				pBus->Unduck(ID(),rItem.item.FadeInTime, rItem.item.FadeCurve);
				break;

			case DuckState_ON:
				pBus->Duck(ID(),rItem.item.DuckVolume, rItem.item.FadeOutTime, rItem.item.FadeCurve);
				break;

			case DuckState_PENDING:
				pBus->PauseDuck(ID());
				break;

			default:
				AKASSERT(!"Unknown Ducking State");
			}
			pBus->Release();
		}
	}
}

AKRESULT CAkBus::RequestDuckNotif()
{
	AKRESULT eResult = AK_Fail; 
	CAkActionDuck* pActionDuck = AkNew(g_DefaultPoolId, CAkActionDuck(AkActionType_Duck,0));
	if(pActionDuck)
	{
		pActionDuck->Delay(m_RecoveryTime);
		pActionDuck->SetElementID(ID());

		AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( NULL ) );
		if(pPendingAction)
		{
			pPendingAction->pAction = pActionDuck;
            pPendingAction->UserParam.CustomParam.customParam = 0;
            pPendingAction->UserParam.CustomParam.ui32Reserved = 0;
            pPendingAction->UserParam.PlayingID = 0;
			g_pAudioMgr->EnqueueOrExecuteAction(pPendingAction);
			eResult = AK_Success;
		}

		pActionDuck->Release();

	}

	return eResult;
}

void CAkBus::CheckDuck()
{
	bool bIsDucked = false;
	for( AkDuckedVolumeList::Iterator iter = m_DuckedVolumeList.Begin(); iter != m_DuckedVolumeList.End(); ++iter )
	{
		// Should be != 0.0f, but the transition result on a DB value, after conversion is innacurate.
		if( (*iter).item.m_EffectiveVolumeOffset < -0.01f )
		{
			bIsDucked = true;
			break;
		}
	}

	if(!bIsDucked)
	{
		MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Unducked, 0, 0 );
	}
}

AkVolumeValue CAkBus::GetDuckedVolume()
{
	AkVolumeValue l_DuckedVolume = AK_MAXIMUM_VOLUME_LEVEL;
	for( AkDuckedVolumeList::Iterator iter = m_DuckedVolumeList.Begin(); iter != m_DuckedVolumeList.End(); ++iter )
	{
		l_DuckedVolume += (*iter).item.m_EffectiveVolumeOffset;
	}

	return l_DuckedVolume;
}

CAkBus* CAkBus::GetMixingBus()
{
	if( HasEffect() && !IsMasterBus() )
	{
		return this;
	}
	return CAkAudioNode::GetMixingBus();
}

CAkBus* CAkBus::GetLimitingBus()
{
	if( m_bIsMaxNumInstOverrideParent )
	{
		return this;
	}
	return CAkAudioNode::GetLimitingBus();
}

AkRTPCFXSubscriptionList* CAkBus::GetFxRTPCSubscriptionList()
{
	return m_pFXChunk ? &( m_pFXChunk->listFXRTPCSubscriptions ) : NULL;
}

#ifndef AK_OPTIMIZED
void CAkBus::UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32			in_ulParamBlockSize
		)
{
	if ( IsMasterBus() )
		CAkURenderer::UpdateMasterBusFxParam( in_FXID, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	else
		CAkURenderer::UpdateBusFxParam( ID(), in_FXID, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
}

void CAkBus::StopMixBus()
{
	if ( IsMasterBus() )
		CAkLEngine::StopAllMixBus( );
	else
		CAkLEngine::StopMixBus( ID() );
}
#endif

bool CAkBus::HasEffect()
{
	if ( m_pFXChunk )
	{
		for ( AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
			if ( m_pFXChunk->aFX[i].id != AK_INVALID_PLUGINID )
				return true;
	}

	return false;
}

bool CAkBus::IsMasterBus()
{
	return ( this == g_pMasterBus );
}

AKRESULT CAkBus::SetInitialParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;
	AkVolumeValue l_VolumeMain			= READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
	m_VolumeMain.SetValue( l_VolumeMain );
	AkVolumeValue l_LFEVolumeMain		= READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
	m_LFEVolumeMain.SetValue( l_LFEVolumeMain );
	m_PitchMain							= (AkInt16)READBANKDATA( AkPitchValue, io_rpData, io_rulDataSize );
	m_LPFMain							= (AkUInt8)READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );
	m_bKillNewest						= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	m_u16MaxNumInstance					= READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
	m_bIsMaxNumInstOverrideParent		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	// Use State is hardcoded to true, not read from banks
	m_bUseState = true;

	// IMPORTANT must read it even if we use it on XBOX360 only... the format is the same
	AkUInt8 l_IsBackgroundMusic = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	// IMPORTANT must read it even if we use it on Wii only... the format is the same
	AkUInt8 l_EnableWiiCompressor = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
#ifdef RVL_OS
	if ( this == g_pMasterBus )
	{
		EnableHardwareCompressor( l_EnableWiiCompressor );
	}
#endif

#ifdef XBOX360
	if( l_IsBackgroundMusic )
	{
		XMP_SetAsXMPBus();
	}
#endif

	return eResult;
}

AKRESULT CAkBus::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUniqueID OverrideBusId = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	if(OverrideBusId)
	{
		CAkAudioNode* pBus = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( OverrideBusId );
		if(pBus)
		{
			eResult = pBus->AddChild( ID() );
			pBus->Release();
		}
		else
		{
			// It is nwo an error to not load the bank content in the proper order.
			MONITOR_ERRORMSG2( L"Master bus structure not loaded: ", L"Make sure that the first bank to be loaded contains the master bus information" );
			eResult = AK_Fail;
		}
	}

	if(eResult == AK_Success)
	{
		SetInitialParams( in_pData, in_ulDataSize );

		//Read and apply StateGroup
		AkStateGroupID ulStateGroupID = READBANKDATA( AkStateGroupID, in_pData, in_ulDataSize );
		SetStateGroup( ulStateGroupID );

		m_RecoveryTime = CAkTimeConv::MillisecondsToSamples( READBANKDATA( AkTimeMs, in_pData, in_ulDataSize ) );
		SetStateSyncType( READBANKDATA( AkUInt32, in_pData, in_ulDataSize ) );

		//Process Child list
		AkUInt32 ulDucks = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		for(AkUInt32 i = 0; i < ulDucks; ++i)
		{
			AkUniqueID BusID	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			AkReal32 DuckVolume	= READBANKDATA( AkReal32, in_pData, in_ulDataSize );
			AkTimeMs FadeOut	= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
			AkTimeMs FadeIn		= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
			AkUInt8 FadeCurve	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

			eResult = AddDuck( BusID, DuckVolume, FadeOut, FadeIn, (AkCurveInterpolation)FadeCurve );

			if(eResult != AK_Success)
			{
				break;
			}
		}
	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialFxParams(in_pData, in_ulDataSize, false);
	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialRTPC(in_pData, in_ulDataSize);
	}

	if(eResult == AK_Success)
	{
		// Read Num States
		AkUInt32 ulNumStates = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

		for( AkUInt32 i = 0 ; i < ulNumStates ; ++i )
		{
			AkBank::AKBKStateItem item = READBANKDATA(AkBank::AKBKStateItem, in_pData, in_ulDataSize );

			if( item.bIsCustom )
			{
				eResult = AddState( item.ID, item.State );
			}
			else
			{
				eResult = LinkStateToStateDefault( item.State );
			}
			if(eResult != AK_Success)
			{
				break;
			}
		}
	}

	if(eResult == AK_Success)
	{
		ReadFeedbackInfo(in_pData, in_ulDataSize);
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkBus::SetInitialFxParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool /*in_bPartialLoadOnly*/ )
{
	AKRESULT eResult = AK_Success;

	// Read Num Fx
	AkUInt32 uNumFx = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AKASSERT(uNumFx <= AK_NUM_EFFECTS_PER_OBJ);
	if ( uNumFx )
	{
		AkUInt32 bitsFXBypass = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		for ( AkUInt32 uFX = 0; uFX < uNumFx; ++uFX )
		{
			AkUInt32 uFXIndex = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			// Read FXID
			AkPluginID fxID = READBANKDATA(AkPluginID, io_rpData, io_rulDataSize);

			if( fxID == AK_PLUGINID_ENVIRONMENTAL )
			{// Keep the EnvironmentMgr posted of the initial state of bypass of the environments
				g_pEnvironmentMgr->BypassAllEnv( (bitsFXBypass & ((1<<0)|(1<<AK_NUM_EFFECTS_BYPASS_ALL_FLAG))) != 0 );
			}

			//
			AkUInt8 l_bIsRendered = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			//We ignore this parameter for bus since FX are never pre-rendered for busses.
			//

			// Read Size
			AkUInt32 ulSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

			if(fxID != AK_INVALID_PLUGINID )
			{
				eResult = SetFX(fxID, uFXIndex, io_rpData, ulSize);
			}

			//Skipping the blob size
			io_rpData += ulSize;
			io_rulDataSize -= ulSize;
		}

		MainBypassFX( bitsFXBypass );
	}

	return eResult;
}

bool CAkBus::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;

	if( ParentBus() )
	{
		return static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes );
	}
	return false;
}

void CAkBus::EnableHardwareCompressor( bool in_Enable )
{
#ifdef RVL_OS
	AXSetCompressor( in_Enable ? AX_COMPRESSOR_ON : AX_COMPRESSOR_OFF );
#endif
}

AKRESULT CAkBus::GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo, CAkRegisteredObj *	in_GameObj /*= NULL*/ )
{
	AKASSERT( in_uFXIndex <  AK_NUM_EFFECTS_PER_OBJ );

	if ( m_pFXChunk )
	{
		out_rFXInfo.EffectTypeID = m_pFXChunk->aFX[ in_uFXIndex ].id;
		out_rFXInfo.pParam = m_pFXChunk->aFX[ in_uFXIndex ].pParam;
		out_rFXInfo.bIsBypassed = GetBypassFX( in_uFXIndex );
	}
	else
	{
		out_rFXInfo.EffectTypeID = AK_INVALID_PLUGINID;
		out_rFXInfo.pParam = NULL;
		out_rFXInfo.bIsBypassed = false;
	}

	out_rFXInfo.FxReferenceID = ID();

	return AK_Success;
}

bool CAkBus::GetBypassFX( AkUInt32 in_uFXIndex )
{
	if ( !m_pFXChunk )
		return false;

	bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> in_uFXIndex ) & 1;

	//We Use RTPC VALUE PRIOR TO ACTIONS!
	if( m_pFXChunk->aFX[ in_uFXIndex ].id != AK_INVALID_PLUGINID && m_RTPCBitArrayMax32.IsSet( RTPC_BypassFX0 + in_uFXIndex ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_BypassFX0 + in_uFXIndex, NULL ) != 0;
	}
	else if( m_pGlobalSIS )
	{
		bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> in_uFXIndex ) & 1;
	}

	return bIsBypass;
}

bool CAkBus::GetBypassAllFX( CAkRegisteredObj * )
{
	if ( !m_pFXChunk )
		return false;

	bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;

	//We Use RTPC VALUE PRIOR TO ACTIONS!
	if( m_RTPCBitArrayMax32.IsSet( RTPC_BypassAllFX ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_BypassAllFX, NULL ) != 0;
	}
	else if( m_pGlobalSIS )
	{
		bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
	}

	return bIsBypass;
}

void CAkBus::ResetFXBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ ) 
{
	if( m_pGlobalSIS )
	{
		m_pGlobalSIS->m_bitsFXBypass = (AkUInt8) ( ( m_pGlobalSIS->m_bitsFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );
	}
}

void CAkBus::SetPitch(
		CAkRegisteredObj *	in_GameObj,
		AkValueMeaning	in_eValueMeaning,
		AkPitchValue		in_TargetValue/* = 0*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_PitchChanged, ID(), AK_INVALID_GAME_OBJECT , in_TargetValue, in_eValueMeaning, in_lTransitionTime );

	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_TargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_TargetValue != m_PitchMain ) ) ) 
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_PitchChanged, ID(), AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisPitchTransitions( GetSIS(), in_TargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkBus::ResetPitch( AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	if( m_pGlobalSIS )
	{
		SetPitch( NULL, AkValueMeaning_Default, 0, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkBus::SetVolume(
		CAkRegisteredObj *	in_GameObj,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_VolumeChanged, ID(), AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_fTargetValue );
	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && VolumeNew != m_VolumeMain ) )
	{
		MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_VolumeChanged, ID(), AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisVolumeTransitions( GetSIS(), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkBus::ResetVolume(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	if(m_pGlobalSIS)
	{
		SetVolume(NULL,AkValueMeaning_Default,0.0f,in_eFadeCurve,in_lTransitionTime);
	}
}

void CAkBus::SetLFE(
		CAkRegisteredObj *	in_GameObj,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LFEChanged, ID(), AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_fTargetValue );
	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && VolumeNew != m_LFEVolumeMain ) )
	{
		MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_LFEChanged, ID(), AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisLFETransitions( GetSIS(), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkBus::ResetLFE( AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	if( m_pGlobalSIS )
	{
		SetLFE( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkBus::SetLPF(
		CAkRegisteredObj *	in_GameObj,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LPFChanged, ID(), AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_LPFMain ) ) ) 
	{
		MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_LPFChanged, ID(), AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisLPFTransitions( GetSIS(), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkBus::ResetLPF( AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	if( m_pGlobalSIS )
	{
		SetLPF( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkBus::Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve /*= AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime /*= 0*/
		)
{
	AKASSERT( g_pRegistryMgr );

	//Only Busses should pass here, an overloaded Mute function exists in CAkParameterNode Object
	AKASSERT( NodeCategory() == AkNodeCategory_Bus || NodeCategory() == AkNodeCategory_FeedbackBus );

	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_Muted, ID(), AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );
	
	MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_Muted, ID(), AK_INVALID_GAME_OBJECT );

	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Muted, 0, 0 );

	if( in_pGameObj == NULL )
	{
		CAkSIS* pSIS = GetSIS();

		StartSisMuteTransitions( pSIS, MUTED_LVL, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkBus::Unmute( CAkRegisteredObj * in_pGameObj, AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	AKASSERT(g_pRegistryMgr);

	//Only Busses should pass here, an overloaded Unmute function exists in CAkParameterNode Object
	AKASSERT( NodeCategory() == AkNodeCategory_Bus || NodeCategory() == AkNodeCategory_FeedbackBus );

	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_Unmuted, ID(), AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );

	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Unmuted, 0, 0 );

	CAkSIS* pSIS = NULL;
	if( in_pGameObj == NULL )
	{
		if(m_pGlobalSIS && m_pGlobalSIS->m_cMuteLevel!= UNMUTED_LVL)
		{
			g_pRegistryMgr->SetNodeIDAsModified(this);
			pSIS = m_pGlobalSIS;
		}
	}
	if(pSIS)
	{
		StartSisMuteTransitions(pSIS,UNMUTED_LVL,in_eFadeCurve,in_lTransitionTime);
	}
}

void CAkBus::UnmuteAll(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	Unmute(NULL,in_eFadeCurve,in_lTransitionTime);
}

CAkSIS* CAkBus::GetSIS( CAkRegisteredObj * in_GameObj )
{
	AKASSERT( g_pRegistryMgr );
	g_pRegistryMgr->SetNodeIDAsModified(this);
	if(!m_pGlobalSIS)
	{
		AkUInt8 bitsBypass = 0;
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			bitsBypass |= ( GetBypassFX( uFXIndex ) ? 1 : 0 ) << uFXIndex;
		m_pGlobalSIS = AkNew( g_DefaultPoolId, CAkSIS( this, bitsBypass ) );
		AKASSERT(m_pGlobalSIS != NULL);
	}

	return m_pGlobalSIS;
}

void CAkBus::RecalcNotification()
{
	if( HasEffect() )
	{
		if ( IsMasterBus() )
		{
			CAkLEngine::ForceMasterBusVolume( GetBusEffectiveVolume() );
			CAkLEngine::ForceMasterBusLFE( GetBusEffectiveLfe() );
		}
		else
		{
			CAkLEngine::ForceBusVolume( ID(), GetBusEffectiveVolume() );
			CAkLEngine::ForceBusLFE( ID(), GetBusEffectiveLfe() );
		}
	}
	for( AkMapChildID::Iterator iter = m_mapChildId.Begin(); iter != m_mapChildId.End(); ++iter )
	{
		if( (*iter).item->IsPlaying() )
		{
			(*iter).item->RecalcNotification();
		}
	}
}

CAkRTPCMgr::SubscriberType CAkBus::GetRTPCSubscriberType() const
{
	return CAkRTPCMgr::SubscriberType_CAkBus;
}


#ifdef XBOX360

CAkBus* CAkBus::m_pXMPBus = NULL;
CAkLock CAkBus::m_XMPLock;

void CAkBus::XMP_MuteBackgroundMusic()
{
	AkAutoLock<CAkLock> gate( m_XMPLock );
	if( m_pXMPBus )
	{
		m_pXMPBus->XMP_Mute();
	}
}
void CAkBus::XMP_UnmuteBackgroundMusic()
{
	AkAutoLock<CAkLock> gate( m_XMPLock );
	if( m_pXMPBus )
	{
		m_pXMPBus->XMP_Unmute();
	}
}

void CAkBus::XMP_Mute()
{
	m_bIsXMPMuted = true;

	AkMutedMapItem item;
    item.m_bIsPersistent = false;
	item.m_bIsGlobal = true;
	item.m_Identifier = this;
	MuteNotification( MUTED_LVL, item, true ); 
}

void CAkBus::XMP_Unmute()
{
	m_bIsXMPMuted = false;

	AkMutedMapItem item;
    item.m_bIsPersistent = false;
	item.m_bIsGlobal = true;
	item.m_Identifier = this;

	AkUInt8 cMutedLevel = UNMUTED_LVL;

	if(m_pGlobalSIS)
	{
		cMutedLevel = m_pGlobalSIS->m_cMuteLevel;
	}

	MuteNotification( cMutedLevel, item, true );
}

void CAkBus::XMP_SetAsXMPBus()
{
	AkAutoLock<CAkLock> gate( m_XMPLock );

	if( !m_bIsXMPBus )
	{
		if( m_pXMPBus == NULL )
		{
			m_pXMPBus = this;
			m_bIsXMPBus = true;

			BOOL bHasTitleXMPControl;
			// Querry the system to get the initial muted state of this bus
			XMPTitleHasPlaybackControl( &bHasTitleXMPControl );
			m_bIsXMPMuted = !(bHasTitleXMPControl);
			if( m_bIsXMPMuted )
			{
				XMP_Mute(); 
			}
		}
		else
		{
			AKASSERT( !"Illegal to have more than one XMP related bus" );
		}
	}
}

void CAkBus::XMP_UnsetAsXMPBus()
{
	AkAutoLock<CAkLock> gate( m_XMPLock );

	if( m_bIsXMPBus )
	{
		// If this assert pops, it means somebody else registered an XMP bus
		AKASSERT( m_pXMPBus == this );
		m_pXMPBus = NULL;
		m_bIsXMPMuted = false;
		m_bIsXMPBus = false;
		XMP_Unmute();
	}
}

#endif
