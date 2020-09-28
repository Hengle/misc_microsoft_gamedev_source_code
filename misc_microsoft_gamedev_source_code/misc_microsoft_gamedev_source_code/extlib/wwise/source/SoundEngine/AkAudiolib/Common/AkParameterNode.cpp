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
// AkParameterNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkStateMgr.h"
#include "AkSIS.h"
#include "AkRegistryMgr.h"
#include "AkActionExcept.h"
#include "AkTransitionManager.h"
#include "AkParentNode.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkRTPCMgr.h"
#include "AkBankFloatConversion.h"
#include "AkModifiers.h"
#include "AkContinuationList.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include "AkAudioMgr.h"
#include "AkActionPlayAndContinue.h"
#include "AkURenderer.h"
#include "Ak3DParams.h"
#include "AkGen3DParams.h"
#include "AkBus.h"
#include "AkLayer.h"

extern AkMemPoolId	g_DefaultPoolId;

CAkParameterNode::CAkParameterNode(AkUniqueID in_ulID)
:CAkParameterNodeBase(in_ulID)
,m_p3DParameters(NULL)
,m_bPositioningInfoOverrideParent( false )
,m_bIsFXOverrideParent( false )
,m_bIsSendOverrideParent( false )
,m_eBelowThresholdBehavior( AkBelowThresholdBehavior_ContinueToPlay )
,m_eVirtualQueueBehavior( AkVirtualQueueBehavior_FromBeginning )
,m_pRangedParams( NULL )
{
	// default path state
	m_PathState.pbPlayed = NULL;
	m_PathState.ulCurrentListIndex = 0;
	m_BaseGenParams.m_fPAN_FR = AK_DEFAULT_PAN_FR_VALUE;
	m_BaseGenParams.m_fPAN_RL = AK_DEFAULT_PAN_RL_VALUE;
	m_BaseGenParams.m_fCenterPct = 0.0f;
	m_BaseGenParams.bIsPannerEnabled = false;
}

CAkParameterNode::~CAkParameterNode()
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		AkDelete( g_DefaultPoolId, (*iter).item );
	}
	DisablePosParams();
	m_mapSIS.Term();
	m_ListPlayCountPerObj.Term();
	if( m_pRangedParams )
	{
		AkDelete( g_DefaultPoolId, m_pRangedParams );
	}
	// get rid of the path played flags if any
	if(m_PathState.pbPlayed != NULL)
	{
		AkFree(g_DefaultPoolId,m_PathState.pbPlayed);
		m_PathState.pbPlayed = NULL;
	}

	m_associatedLayers.Term();
}

void CAkParameterNode::Volume(AkVolumeValue in_Volume, AkVolumeValue in_MinRangeValue/*=0*/, AkVolumeValue in_MaxRangeValue/*=0*/)
{
	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_Volume );

	if( DoesChangeMustBeNotified( PT_Volume ) )
	{
		VolumeNotification( VolumeNew.GetValue() - m_VolumeMain.GetValue() );
	}
	m_VolumeMain = VolumeNew;
	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->Volume.m_min = in_MinRangeValue;
			m_pRangedParams->Volume.m_max = in_MaxRangeValue;
		}
	}
}

void CAkParameterNode::LFEVolume( AkVolumeValue in_LFEVolume, AkVolumeValue in_MinRangeValue, AkVolumeValue in_MaxRangeValue )
{
	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_LFEVolume );

	if( DoesChangeMustBeNotified( PT_LFE ) )
	{
		LFENotification( VolumeNew.GetValue()  - m_LFEVolumeMain.GetValue() );
	}
	m_LFEVolumeMain = VolumeNew;
	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->LFE.m_min = in_MinRangeValue;
			m_pRangedParams->LFE.m_max = in_MaxRangeValue;
		}
	}
}

void CAkParameterNode::Pitch(AkPitchValue in_Pitch, AkPitchValue in_MinRangeValue, AkPitchValue in_MaxRangeValue)
{
	if( DoesChangeMustBeNotified( PT_Pitch ) )
	{
		PitchNotification( in_Pitch  - m_PitchMain );
	}
	AKASSERT( in_Pitch >= -4800 && in_Pitch <= 4800 );
	m_PitchMain = (AkInt16)in_Pitch;
	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->Pitch.m_min = in_MinRangeValue;
			m_pRangedParams->Pitch.m_max = in_MaxRangeValue;
		}
	}
}

void CAkParameterNode::LPF( AkLPFType in_LPF, AkLPFType in_MinRangeValue, AkLPFType in_MaxRangeValue )
{
	if( DoesChangeMustBeNotified( PT_LPF ) )
	{
		LPFNotification( in_LPF  - m_LPFMain );
	}
	AKASSERT( in_LPF >= 0 && in_LPF <= 100 );
	m_LPFMain = (AkUInt8)in_LPF;
	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->LPF.m_min = in_MinRangeValue;
			m_pRangedParams->LPF.m_max = in_MaxRangeValue;
		}
	}
}

void CAkParameterNode::FeedbackVolume(AkVolumeValue in_Volume, AkVolumeValue in_MinRangeValue, AkVolumeValue in_MaxRangeValue)
{
	CAkParameterNodeBase::FeedbackVolume(in_Volume);

	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->FeedbackVolume.m_min = in_MinRangeValue;
			m_pRangedParams->FeedbackVolume.m_max = in_MaxRangeValue;
		}
	}
}
void CAkParameterNode::FeedbackLPF( AkLPFType in_FeedbackLPF, AkLPFType in_MinRangeValue, AkLPFType in_MaxRangeValue)
{
	CAkParameterNodeBase::FeedbackLPF(in_FeedbackLPF);

	if( in_MinRangeValue || in_MaxRangeValue || m_pRangedParams )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->FeedbackLPF.m_min = in_MinRangeValue;
			m_pRangedParams->FeedbackLPF.m_max = in_MaxRangeValue;
		}
	}
}

//====================================================================================================
// check parents for 3D params
//====================================================================================================
void CAkParameterNode::Get3DParams( CAkGen3DParams*& in_rp3DParams, CAkRegisteredObj * in_GameObj, bool in_bUpdateOnly, BaseGenParams * io_pBasePosParams )
{
	CAkAudioNode* pAudioNode = this;

	while( pAudioNode != NULL )
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pParameterNode->Get3DCloneForObject( in_GameObj, in_rp3DParams, in_bUpdateOnly );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_p3DParameters
		 || pParameterNode->m_bPositioningInfoOverrideParent
		 || pAudioNode == NULL )
		{
			pParameterNode->UpdateBaseParamsFromRTPC( in_GameObj, io_pBasePosParams );
			break;
		}
	}
}

AKRESULT CAkParameterNode::GetStatic3DParams( AkPositioningInfo& out_rPosInfo )
{
	CAkAudioNode* pAudioNode = this;

	memset( &out_rPosInfo, 0, sizeof( AkPositioningInfo ) ); //clean output structure

	while( pAudioNode != NULL )
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_p3DParameters
			|| pParameterNode->m_bPositioningInfoOverrideParent
			|| pAudioNode == NULL )
		{
			//Copy 3D params
			if( pParameterNode->m_p3DParameters )
			{
				Gen3DParams* p3DParams = pParameterNode->m_p3DParameters->GetParams();
				out_rPosInfo.positioningType = p3DParams->m_eType;
				out_rPosInfo.bUpdateEachFrame = p3DParams->m_bIsDynamic;
				out_rPosInfo.bUseSpatialization = p3DParams->m_bIsSpatialized;

				//attenuation info
				AkUniqueID AttenuationID = pParameterNode->m_p3DParameters->GetParams()->m_uAttenuationID;
				CAkAttenuation* pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( AttenuationID );
				if( pAttenuation )
				{
					out_rPosInfo.bUseAttenuation = true;
					out_rPosInfo.bUseConeAttenuation = pAttenuation->m_bIsConeEnabled;
					if( pAttenuation->m_bIsConeEnabled )
					{
						out_rPosInfo.fInnerAngle = pAttenuation->m_ConeParams.fInsideAngle;
						out_rPosInfo.fOuterAngle = pAttenuation->m_ConeParams.fOutsideAngle;  //convert to degrees?
						out_rPosInfo.fConeMaxAttenuation = pAttenuation->m_ConeParams.fOutsideVolume; //convert to degrees?
						out_rPosInfo.LPFCone = pAttenuation->m_ConeParams.LoPass;
					}

					CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
					AKASSERT( pVolumeDryCurve );
					out_rPosInfo.fMaxDistance = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize-1].From;
					out_rPosInfo.fVolDryAtMaxDist = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize-1].To;
					
					CAkAttenuation::AkAttenuationCurve* pVolumeWetCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeWet );
					if(pVolumeWetCurve)
						out_rPosInfo.fVolWetAtMaxDist = pVolumeWetCurve->m_pArrayGraphPoints[pVolumeWetCurve->m_ulArraySize-1].To; 

					CAkAttenuation::AkAttenuationCurve* pLPFCurve = pAttenuation->GetCurve( AttenuationCurveID_LowPassFilter );
					if(pLPFCurve)
						out_rPosInfo.LPFValueAtMaxDist = pLPFCurve->m_pArrayGraphPoints[pLPFCurve->m_ulArraySize-1].To; 

					pAttenuation->Release();
				}
			}
			else
			{
				out_rPosInfo.positioningType = Ak2DPositioning;
			}

			//Copy base params
			out_rPosInfo.fCenterPct = pParameterNode->m_BaseGenParams.m_fCenterPct;
			return AK_Success;
		}
	}

	return AK_IDNotFound;
}

bool CAkParameterNode::UpdateBaseParams( CAkRegisteredObj * in_GameObj, BaseGenParams * io_pBasePosParams )
{
	CAkAudioNode* pAudioNode = this;

	while( pAudioNode != NULL )
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_bPositioningInfoOverrideParent
		 || pAudioNode == NULL )
		{
			return pParameterNode->UpdateBaseParamsFromRTPC( in_GameObj, io_pBasePosParams );
		}
	}
	return false;
}

bool CAkParameterNode::GetMaxRadius( AkReal32 & out_fRadius )
{
	CAkAudioNode* pAudioNode = this;

	while( pAudioNode != NULL )
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		if ( pParameterNode->m_p3DParameters )
		{
			bool bReturnValue = true;
			AkUniqueID AttenuationID = pParameterNode->m_p3DParameters->GetParams()->m_uAttenuationID;
			CAkAttenuation* pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( AttenuationID );
			if( pAttenuation )
			{
				CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
				if( pVolumeDryCurve )
					out_fRadius = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From;
				else
					bReturnValue = false; //no attenuation
				pAttenuation->Release();
			}
			else
				bReturnValue = false;

			return bReturnValue;
		}

		if( pParameterNode->m_bPositioningInfoOverrideParent )
			break;

		pAudioNode = pAudioNode->Parent();
	}

	return false;
}
AKRESULT CAkParameterNode::GetAudioParameters(AkSoundParams &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_GameObjPtr, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/)
{
	AKRESULT eResult = AK_Success;
	AkUInt32 ulParamSelect = in_ulParamSelect;
	if(m_bUseState && m_pStateTransitionInfo )
	{
		CAkState* pState = GetState();
		if(pState != NULL)
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
					in_ulParamSelect = (in_ulParamSelect & ~PT_Pitch);
				}
				io_Parameters.Pitch += m_pStateTransitionInfo->m_Pitch;
			}
			if(in_ulParamSelect & PT_LPF)
			{
				if(pState->LPFMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect = (in_ulParamSelect & ~PT_LPF);
				}
				io_Parameters.LPF += m_pStateTransitionInfo->m_LPF;
			}
			if(in_ulParamSelect & PT_LFE)
			{
				if(pState->LFEVolumeMeaning() == AkValueMeaning_Independent)
				{
					in_ulParamSelect = (in_ulParamSelect & ~PT_LFE);
				}
				io_Parameters.LFE += m_pStateTransitionInfo->m_Lfe;
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
			if(in_ulParamSelect & PT_LPF)
			{
				io_Parameters.LPF += m_pStateTransitionInfo->m_LPF;
			}
			if(in_ulParamSelect & PT_LFE)
			{
				io_Parameters.LFE += m_pStateTransitionInfo->m_Lfe;
			}
		}
	}

	if(in_ulParamSelect & PT_Volume)
	{
		io_Parameters.Volume += m_VolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Volume ) )
		{
			io_Parameters.Volume += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Volume, in_GameObjPtr );
		}
	}
	if(in_ulParamSelect & PT_Pitch)
	{
		io_Parameters.Pitch += m_PitchMain;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Pitch ) )
		{
			io_Parameters.Pitch += (AkPitchValue)g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Pitch, in_GameObjPtr );
		}
	}
	if(in_ulParamSelect & PT_LPF)
	{
		io_Parameters.LPF += m_LPFMain;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_LPF ) )
		{
			io_Parameters.LPF += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_LPF, in_GameObjPtr );
		}
	}
	if(in_ulParamSelect & PT_LFE)
	{
		io_Parameters.LFE += m_LFEVolumeMain.GetValue();
		if( m_RTPCBitArrayMax32.IsSet( RTPC_LFE ) )
		{
			io_Parameters.LFE += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_LFE, in_GameObjPtr );
		}
	}
	if(m_pGlobalSIS)
	{
		io_Parameters.Volume += m_pGlobalSIS->m_EffectiveVolumeOffset;
		io_Parameters.LFE += m_pGlobalSIS->m_EffectiveLFEOffset;
		io_Parameters.LPF += m_pGlobalSIS->m_EffectiveLPFOffset;
		io_Parameters.Pitch += m_pGlobalSIS->m_EffectivePitchOffset;
		if(m_pGlobalSIS->m_cMuteLevel != UNMUTED_LVL)
		{
			AkMutedMapItem item;
            item.m_bIsPersistent = false;
			item.m_bIsGlobal = true;
			item.m_Identifier = this;
			io_rMutedMap.Set( item, m_pGlobalSIS->m_cMuteLevel );
		}
	}

	CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
	if( l_ppSIS )
	{
		CAkSIS* pSIS = *l_ppSIS;
		io_Parameters.Volume += pSIS->m_EffectiveVolumeOffset;
		io_Parameters.LFE += pSIS->m_EffectiveLFEOffset;
		io_Parameters.LPF += pSIS->m_EffectiveLPFOffset;
		io_Parameters.Pitch += pSIS->m_EffectivePitchOffset;
		if(pSIS->m_cMuteLevel != UNMUTED_LVL)
		{
			AkMutedMapItem item;
            item.m_bIsPersistent = false;
			item.m_bIsGlobal = false;
			item.m_Identifier = this;
			io_rMutedMap.Set( item, pSIS->m_cMuteLevel );
		}
	}
	if(in_bIncludeRange)
	{
		if( m_pRangedParams )
		{
			io_Ranges.LFEOffset += RandomizerModifier::GetMod( m_pRangedParams->LFE );
			io_Ranges.PitchOffset += RandomizerModifier::GetMod( m_pRangedParams->Pitch );
			io_Ranges.LPFOffset += RandomizerModifier::GetMod( m_pRangedParams->LPF );
			io_Ranges.VolumeOffset += RandomizerModifier::GetMod( m_pRangedParams->Volume );
		}
	}

	if(in_bDoBusCheck && m_pBusOutputNode)
	{
		if(m_pParentNode != NULL)
		{
			static_cast<CAkParameterNodeBase*>(m_pParentNode)->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, false);
		}

		static_cast<CAkParameterNodeBase*>(m_pBusOutputNode)->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, false);

	}
	else
	{
		if(m_pParentNode != NULL)
		{
			static_cast<CAkParameterNodeBase*>(m_pParentNode)->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, in_bDoBusCheck);
		}
	}

	for ( LayerList::Iterator it = m_associatedLayers.Begin(), itEnd = m_associatedLayers.End();
		  it != itEnd;
		  ++it )
	{
		(*it)->GetAudioParameters( this, io_Parameters, ulParamSelect, io_rMutedMap, in_GameObjPtr );
	}

	return eResult;
}

void CAkParameterNode::SetPitch(
		CAkRegisteredObj *	in_GameObjPtr,
		AkValueMeaning	in_eValueMeaning,
		AkPitchValue		in_TargetValue /*= 0*/,
		AkCurveInterpolation		in_eFadeCurve /*= AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime /*= 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_PitchChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_TargetValue, in_eValueMeaning, in_lTransitionTime );

	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_TargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_TargetValue != m_PitchMain ) ) ) 
	{
		MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_PitchChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisPitchTransitions( GetSIS( in_GameObjPtr ), in_TargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkParameterNode::SetVolume(
		CAkRegisteredObj *	in_GameObjPtr,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_VolumeChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_fTargetValue );
	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && VolumeNew != m_VolumeMain ) )
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_VolumeChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisVolumeTransitions( GetSIS( in_GameObjPtr ), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkParameterNode::SetLFE(
		CAkRegisteredObj *	in_GameObjPtr,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LFEChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	AkCompactedVolume VolumeNew;
	VolumeNew.SetValue( in_fTargetValue );
	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && VolumeNew != m_LFEVolumeMain ) )
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_LFEChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisLFETransitions( GetSIS( in_GameObjPtr ), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkParameterNode::SetLPF(
		CAkRegisteredObj *	in_GameObjPtr,
		AkValueMeaning	in_eValueMeaning,
		AkReal32			in_fTargetValue/* = 0.0f*/,
		AkCurveInterpolation		in_eFadeCurve/* = AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime/* = 0*/
		)
{
#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LPFChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_LPFMain ) ) ) 
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_LPFChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisLPFTransitions( GetSIS( in_GameObjPtr ), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkParameterNode::Mute(
		CAkRegisteredObj *	in_GameObjPtr,
		AkCurveInterpolation		in_eFadeCurve /*= AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime /*= 0*/
		)
{
	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_Muted, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );
	
	MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_Muted, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );

	CAkSIS* pSIS = GetSIS( in_GameObjPtr );

	StartSisMuteTransitions(pSIS,MUTED_LVL,in_eFadeCurve,in_lTransitionTime);
}

void CAkParameterNode::Unmute( CAkRegisteredObj * in_GameObjPtr, AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	AKASSERT(g_pRegistryMgr);
	
	MONITOR_SETPARAMNOTIF_LONG( AkMonitorData::NotificationReason_Unmuted, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );

	CAkSIS* pSIS = NULL;
	if( in_GameObjPtr != NULL )
	{
		CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
		if( l_ppSIS )
		{
			in_GameObjPtr->SetNodeAsModified(this);
			pSIS = *l_ppSIS;
		}
	}
	else
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

void CAkParameterNode::UnmuteAllObj(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_cMuteLevel != UNMUTED_LVL )
		{
			Unmute( (*iter).item->m_pGameObj, in_eFadeCurve,in_lTransitionTime );
		}
	}
}

void CAkParameterNode::UnmuteAll(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	Unmute(NULL,in_eFadeCurve,in_lTransitionTime);
	UnmuteAllObj(in_eFadeCurve,in_lTransitionTime);
}

void CAkParameterNode::Unregister(CAkRegisteredObj * in_GameObjPtr)
{
	AkMapSIS::Iterator iter = m_mapSIS.Begin();
	while( iter != m_mapSIS.End() )
	{
		if( (*iter).key == in_GameObjPtr )
		{
			if( (*iter).item )
			{
				AkDelete( g_DefaultPoolId, (*iter).item );
			}
			iter = m_mapSIS.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void CAkParameterNode::ResetPitch(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_EffectivePitchOffset )
		{
			SetPitch( (*iter).item->m_pGameObj, AkValueMeaning_Default );
		}
	}

	if(m_pGlobalSIS)
	{
		SetPitch( NULL, AkValueMeaning_Default, 0, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkParameterNode::ResetVolume(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_EffectiveVolumeOffset)
		{
			SetVolume( (*iter).item->m_pGameObj, AkValueMeaning_Default ,0.0f, in_eFadeCurve, in_lTransitionTime );
		}
	}

	if(m_pGlobalSIS)
	{
		SetVolume( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkParameterNode::ResetFeedbackVolume(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_EffectiveFeedbackVolumeOffset)
		{
			SetFeedbackVolume( (*iter).item->m_pGameObj, AkValueMeaning_Default ,0.0f, in_eFadeCurve, in_lTransitionTime );
		}
	}

	if(m_pGlobalSIS)
	{
		SetFeedbackVolume( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkParameterNode::ResetLFE(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_EffectiveLFEOffset)
		{
			SetLFE( (*iter).item->m_pGameObj, AkValueMeaning_Default ,0.0f, in_eFadeCurve, in_lTransitionTime );
		}
	}

	if(m_pGlobalSIS)
	{
		SetLFE( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkParameterNode::ResetLPF( AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		if( (*iter).item->m_EffectiveLPFOffset)
		{
			SetLPF( (*iter).item->m_pGameObj, AkValueMeaning_Default ,0.0f, in_eFadeCurve, in_lTransitionTime );
		}
	}

	if(m_pGlobalSIS)
	{
		SetLPF( NULL, AkValueMeaning_Default, 0.0f, in_eFadeCurve, in_lTransitionTime );
	}
}

AKRESULT CAkParameterNode::PlayAndContinueAlternate( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Fail;

	// Set history ready for next
	AkUInt32& ulrCount = in_rPBIParams.playHistory.HistArray.uiArraySize;

	while( ulrCount )
	{
		if( in_rPBIParams.playHistory.IsContinuous( ulrCount -1 ) )
		{
			break;
		}
		else
		{
			--ulrCount;
		}
	}

	//Determine next
	AkUniqueID			NextElementToPlayID	= AK_INVALID_UNIQUE_ID;
	AkTransitionMode	l_eTransitionMode	= Transition_Disabled;
	AkTimeMs			l_TransitionTime	= 0;
	AkUInt16			wPositionSelected	= 0;

    AKASSERT( in_rPBIParams.pContinuousParams );
    while( !in_rPBIParams.pContinuousParams->spContList->m_listItems.IsEmpty() )
	{
        CAkContinueListItem & item = in_rPBIParams.pContinuousParams->spContList->m_listItems.Last();
		if( !( item.m_pMultiPlayNode ) )
		{
            CAkAudioNode* pNode = item.m_pContainer->GetNextToPlayContinuous( in_rPBIParams.pGameObj, wPositionSelected, item.m_pContainerInfo, item.m_LoopingInfo );
			if(pNode)
			{
                in_rPBIParams.playHistory.HistArray.aCntrHist[ in_rPBIParams.playHistory.HistArray.uiArraySize - 1 ] = wPositionSelected;
				NextElementToPlayID = pNode->ID();
				pNode->Release();
				l_eTransitionMode = item.m_pContainer->TransitionMode();
				l_TransitionTime = item.m_pContainer->TransitionTime();
				break;
			}
			else
			{
                in_rPBIParams.playHistory.RemoveLast();
				while( in_rPBIParams.playHistory.HistArray.uiArraySize
                    && !in_rPBIParams.playHistory.IsContinuous( in_rPBIParams.playHistory.HistArray.uiArraySize - 1 ) )
				{
					in_rPBIParams.playHistory.RemoveLast();
				}
				in_rPBIParams.pContinuousParams->spContList->m_listItems.RemoveLast();
			}
		}
		else // Encountered a switch block
		{
			in_rPBIParams.pContinuousParams->spContList->m_listItems.RemoveLast();
			in_rPBIParams.pContinuousParams->spContList = item.m_pMultiPlayNode->ContGetList( item.m_pAlternateContList );
			
			if( !in_rPBIParams.pContinuousParams->spContList )
			{
				eResult = AK_PartialSuccess;
				break;
			}
		}
	}

	//Then launch next if there is a next
	if( NextElementToPlayID != AK_INVALID_UNIQUE_ID )
	{
		// create the action we need
		CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, in_rPBIParams.pContinuousParams->spContList );
		if(pAction)
		{
			pAction->SetPauseCount( in_rPBIParams.pContinuousParams->ulPauseCount );
			pAction->SetHistory( in_rPBIParams.playHistory );
			pAction->SetElementID( NextElementToPlayID );
            pAction->SetInstigator( in_rPBIParams.pInstigator );

            AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( in_rPBIParams.pGameObj ) );
			if( pPendingAction )
			{
                pAction->SetSAInfo( in_rPBIParams.sequenceID );
                pAction->SetIsFirstPlay( in_rPBIParams.bIsFirst );
                pAction->SetInitialPlaybackState( in_rPBIParams.ePlaybackState );

				// copy the transitions we have
				if (
					pAction->SetPlayStopTransition( in_rPBIParams.pContinuousParams->pPlayStopTransition, in_rPBIParams.pContinuousParams->bIsPlayStopTransitionFading, pPendingAction ) == AK_Success
					&&
					pAction->SetPauseResumeTransition( in_rPBIParams.pContinuousParams->pPauseResumeTransition, in_rPBIParams.pContinuousParams->bIsPauseResumeTransitionFading, pPendingAction ) == AK_Success
					)
				{
                    pAction->SetPathInfo( in_rPBIParams.pContinuousParams->pPathInfo );

					if(l_eTransitionMode == Transition_Delay)
					{
						pAction->Delay( CAkTimeConv::MillisecondsToSamples( l_TransitionTime ) );
					}
					else
					{
						pAction->Delay( AK_NUM_VOICE_REFILL_FRAMES * AK_WAIT_BUFFERS_AFTER_PLAY_FAILED ); //WG-2352: avoid freeze on loop
																		                                  //WG-4724: Delay must be exactly the size of a
							                                                                              //         buffer to avoid sample accurate glitches
							                                                                              //         and Buffer inconsistencies
					}

					pPendingAction->pAction = pAction;
					pPendingAction->UserParam = in_rPBIParams.userParams;

					g_pAudioMgr->EnqueueOrExecuteAction( pPendingAction );

					//If we get here, it means we succeeded in forcing next.
					eResult = AK_Success;
				}
				else
				{
					AkDelete( g_DefaultPoolId, pPendingAction );
				}
			}

			// we are done with these
			pAction->Release();
		}
	}
	if ( in_rPBIParams.pContinuousParams->spContList && eResult != AK_Success && eResult != AK_PartialSuccess )
	{
		in_rPBIParams.pContinuousParams->spContList = NULL;
	}

	if( eResult != AK_Success && eResult != AK_PartialSuccess )
	{
		MONITOR_OBJECTNOTIF( in_rPBIParams.userParams.PlayingID, in_rPBIParams.pGameObj->ID(), in_rPBIParams.userParams.CustomParam, AkMonitorData::NotificationReason_ContinueAborted, in_rPBIParams.playHistory.HistArray, ID(), 0 );
	}

	return eResult;
}

AKRESULT CAkParameterNode::SetInitialParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;
	
	AkVolumeValue fReadValue2;
	AkVolumeValue fReadValue3;

	AkPitchValue ReadValue2;
	AkPitchValue ReadValue3;

	//Read Volume
	AkVolumeValue l_VolumeMain = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
	m_VolumeMain.SetValue( l_VolumeMain );

	fReadValue2 = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );

	fReadValue3 = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );

	if( fReadValue2 || fReadValue3 )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->Volume.m_min = fReadValue2;
			m_pRangedParams->Volume.m_max = fReadValue3;
		}
	}

	//Read Lfe
	AkVolumeValue l_LFEVolumeMain = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
	m_LFEVolumeMain.SetValue( l_LFEVolumeMain );

	fReadValue2 = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );

	fReadValue3 = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );

	if( fReadValue2 || fReadValue3 )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->LFE.m_min = fReadValue2;
			m_pRangedParams->LFE.m_max = fReadValue3;
		}
	}

	//Read Pitch
	m_PitchMain = (AkInt16)READBANKDATA( AkPitchValue, io_rpData, io_rulDataSize );

	ReadValue2 = READBANKDATA( AkPitchValue, io_rpData, io_rulDataSize );

	ReadValue3 = READBANKDATA( AkPitchValue, io_rpData, io_rulDataSize );

	if( ReadValue2 || ReadValue3 )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->Pitch.m_min = ReadValue2;
			m_pRangedParams->Pitch.m_max = ReadValue3;
		}
	}

	//Read LPF
	m_LPFMain = (AkUInt8)READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );

	fReadValue2 = READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );

	fReadValue3 = READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );

	if( fReadValue2 || fReadValue3 )
	{
		if( EnableRangeParams() )
		{
			m_pRangedParams->LPF.m_min = fReadValue2;
			m_pRangedParams->LPF.m_max = fReadValue3;
		}
	}

	// Use State is hardcoded to true, not read from banks
	m_bUseState = true;

	//Read and apply StateGroup
	AkStateGroupID ulStateGroupID = READBANKDATA( AkStateGroupID, io_rpData, io_rulDataSize );
	SetStateGroup( ulStateGroupID );

	return eResult;
}

AKRESULT CAkParameterNode::SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly)
{
	AKRESULT eResult = AK_Success;

	// Read Num Fx
	AkUInt8 bIsOverrideParentFX = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	if(!in_bPartialLoadOnly)
		m_bIsFXOverrideParent = bIsOverrideParentFX ? true : false;

	AkUInt32 uNumFx = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AKASSERT( uNumFx <= AK_NUM_EFFECTS_PER_OBJ );
	if ( uNumFx )
	{
		AkUInt32 bitsFXBypass = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		for ( AkUInt32 uFX = 0; uFX < uNumFx; ++uFX )
		{
			AkUInt32 uFXIndex = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

			// Read FXID
			AkPluginID fxID = READBANKDATA(AkPluginID, io_rpData, io_rulDataSize);

			AkUInt8 bIsRendered = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			RenderedFX( uFXIndex, ( bIsRendered != 0 ) );

			// Read Size
			AkUInt32 ulSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
			if(fxID != AK_INVALID_PLUGINID && 
			   !in_bPartialLoadOnly)
			{
				eResult = SetFX(fxID, uFXIndex, io_rpData, ulSize);
			}

			//Skipping the blob size
			io_rpData += ulSize;
			io_rulDataSize -= ulSize;
		}

		if(!in_bPartialLoadOnly)
			MainBypassFX( bitsFXBypass );
	}

	return eResult;
}

void CAkParameterNode::OverrideFXParent( bool in_bIsFXOverrideParent )
{
#ifndef AK_OPTIMIZED
	if ( m_pFXChunk )
	{
		for( int i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
		{
			if( m_pFXChunk->aFX[ i ].bRendered )
				return;
		}
	}
#endif
	m_bIsFXOverrideParent = in_bIsFXOverrideParent;
}

//////////////////////////////////////////////////////////////////////////////
//Positioning information setting
//////////////////////////////////////////////////////////////////////////////

AKRESULT CAkParameterNode::Enable3DPosParams()
{
	AKRESULT eResult = AK_Success;
	if( !m_p3DParameters )
	{	
		m_bPositioningInfoOverrideParent = true;
		CAkGen3DParams * p3DParams = AkNew( g_DefaultPoolId, CAkGen3DParams() );
		if(!p3DParams)
		{
			eResult = AK_InsufficientMemory;
		}
		else
		{
			p3DParams->SetPathOwner(ID());
			m_p3DParameters = p3DParams;
		}
	}
#ifndef AK_OPTIMIZED
	else
	{
		InvalidatePaths();
	}
#endif
	return eResult;
}

AKRESULT CAkParameterNode::Enable2DPosParams()
{
	AKRESULT eResult = AK_Success;
	DisablePosParams();
	m_bPositioningInfoOverrideParent = true;

	return eResult;
}

void CAkParameterNode::DisablePosParams()
{
	m_bPositioningInfoOverrideParent = false;
#ifndef AK_OPTIMIZED
	InvalidatePaths();
#endif
	if( m_p3DParameters )
	{
		m_p3DParameters->Term();
		AkDelete( g_DefaultPoolId, m_p3DParameters );
		m_p3DParameters = NULL;
	}
}

AKRESULT CAkParameterNode::PosSetPositioningType( AkPositioningType in_ePosType )
{
	AKRESULT eResult;

	switch( in_ePosType )
	{
	case AkUndefined:
		DisablePosParams();
		return AK_Success;

	case Ak2DPositioning:
		Enable2DPosParams();
		return AK_Success;

	case Ak3DGameDef:
	case Ak3DUserDef:
		if( !m_p3DParameters )
			Enable3DPosParams();
		break;
	}

	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetPositioningType( in_ePosType );
		SetParamComplex( POSID_PositioningType, (AkReal32)in_ePosType );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters couldn't be allocated" );
	}

	return eResult;
}

AKRESULT CAkParameterNode::PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled )
{
	AKRESULT eResult = AK_Success;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetSpatializationEnabled( in_bIsSpatializationEnabled );
//		SetParamComplex( POSID_Positioning_SpatializationEnabled, (AkReal32)in_bIsSpatializationEnabled );
	}

	return eResult;
}

AKRESULT CAkParameterNode::PosSetAttenuationID( AkUniqueID in_AttenuationID )
{
	AKRESULT eResult = AK_Success;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetAttenuationID( in_AttenuationID );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetConeUsage( bool in_bIsConeEnabled )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetConeUsage( in_bIsConeEnabled );
		SetParamComplex( POSID_Positioning_Cone_Attenuation_ON_OFF, (AkReal32)in_bIsConeEnabled );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetCenterPct( AkInt in_iCenterPct )
{
	m_BaseGenParams.m_fCenterPct = in_iCenterPct * 0.01f;
	SetParamComplex( POSID_Positioning_Divergence_Center_PCT, m_BaseGenParams.m_fCenterPct );
	return AK_Success;
}

AKRESULT CAkParameterNode::PosSetPAN_RL( AkReal32 in_fPanRL )
{
	m_BaseGenParams.m_fPAN_RL = in_fPanRL;
	SetParamComplex( POSID_Position_PAN_RL, in_fPanRL );

	return AK_Success;
}

AKRESULT CAkParameterNode::PosSetPAN_FR( AkReal32 in_fPanFR )
{
	m_BaseGenParams.m_fPAN_FR = in_fPanFR;
	SetParamComplex( POSID_Position_PAN_FR, in_fPanFR );

	return AK_Success;
}

AKRESULT CAkParameterNode::PosSetPannerEnabled( bool in_bIsPannerEnabled )
{
	m_BaseGenParams.bIsPannerEnabled = in_bIsPannerEnabled;
	SetParamComplex( POSID_2DPannerEnabled, in_bIsPannerEnabled );

	return AK_Success;
}


AKRESULT CAkParameterNode::PosSetIsPositionDynamic( bool in_bIsDynamic )
{
	AKRESULT eResult = AK_Success;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetIsPositionDynamic( in_bIsDynamic );
		SetParamComplex( POSID_IsPositionDynamic, (AkReal32)in_bIsDynamic );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetPathMode( AkPathMode in_ePathMode )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		// get rid of the path played flags if any
		if(m_PathState.pbPlayed != NULL)
		{
			AkFree(g_DefaultPoolId,m_PathState.pbPlayed);
			m_PathState.pbPlayed = NULL;
		}
		eResult = m_p3DParameters->SetPathMode( in_ePathMode );
		SetParamComplex( POSID_PathMode, (AkReal32)in_ePathMode );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetIsLooping( bool in_bIsLooping )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetIsLooping( in_bIsLooping );
		SetParamComplex( POSID_IsLooping, (AkReal32)in_bIsLooping );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetTransition( AkTimeMs in_TransitionTime )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetTransition( in_TransitionTime );
		SetParamComplex( POSID_Transition, (AkReal32)in_TransitionTime );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetPath(
	AkPathVertex*           in_pArrayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	AKRESULT eResult;
#ifndef AK_OPTIMIZED
	InvalidatePaths();
#endif

	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetPath( in_pArrayVertex, in_ulNumVertices, in_pArrayPlaylist, in_ulNumPlaylistItem );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkParameterNode::PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		)
{
	if( m_p3DParameters )
	{
		m_p3DParameters->UpdatePathPoint( in_ulPathIndex, in_ulVertexIndex, in_newPosition, in_DelayToNext );
	}
	else
	{
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void CAkParameterNode::Get3DCloneForObject( CAkRegisteredObj * in_GameObj, CAkGen3DParams*& in_rp3DParams, bool in_bUpdateOnly )
{
	AKASSERT( in_GameObj );

	if(m_p3DParameters)
	{
		if(in_rp3DParams == NULL)
		{
			in_rp3DParams = AkNew( g_DefaultPoolId, CAkGen3DParams() );
		}
		if( in_rp3DParams )
		{
			if(!in_bUpdateOnly)
			{
				// Copying parameters. Will copy the array pointer for the path.
				*in_rp3DParams = *m_p3DParameters;
			}
		}
	}
}

bool CAkParameterNode::UpdateBaseParamsFromRTPC( CAkRegisteredObj * in_GameObj, BaseGenParams* in_pBasePosParams )
{
	AKASSERT( in_pBasePosParams );
	AKASSERT( g_pRTPCMgr );

	bool bUsingRTPC = false;

	*in_pBasePosParams = m_BaseGenParams;
	if( !m_RTPCBitArrayMax32.IsEmpty() )
	{
		AkReal32 fMaxRadius = 0.0f;
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Position_PAN_RL ) )
		{
			bUsingRTPC = true;
			in_pBasePosParams->m_fPAN_RL = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_RL, in_GameObj );
			if( m_p3DParameters )
			{
				if( GetMaxRadius( fMaxRadius ) )
					in_pBasePosParams->m_fPAN_RL *= fMaxRadius / 100.0f;
			}
		}
		if( m_RTPCBitArrayMax32.IsSet( RTPC_Position_PAN_FR ) )
		{
			bUsingRTPC = true;
			in_pBasePosParams->m_fPAN_FR = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_Position_PAN_FR, in_GameObj );
			if( m_p3DParameters )
			{
				if( fMaxRadius != 0.0f || GetMaxRadius( fMaxRadius ) )
					in_pBasePosParams->m_fPAN_RL *= fMaxRadius / 100.0f;
			}
		}
	}

	return bUsingRTPC;
}

CAkSIS* CAkParameterNode::GetSIS( CAkRegisteredObj * in_GameObjPtr )
{
	AKASSERT(g_pRegistryMgr);
	CAkSIS* l_pSIS = NULL;
	if( in_GameObjPtr != NULL)
	{
		in_GameObjPtr->SetNodeAsModified(this);
		CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
		if(l_ppSIS)
		{
			l_pSIS = *l_ppSIS;
		}
		else
		{
			l_pSIS = AkNew( g_DefaultPoolId, CAkSIS( this, in_GameObjPtr, GetBypassFX( 0, in_GameObjPtr ) ) ); // FIXME
			if( l_pSIS )
			{
				if( !m_mapSIS.Set( in_GameObjPtr, l_pSIS ) )
				{
					AkDelete( g_DefaultPoolId, l_pSIS );
					l_pSIS = NULL;
				}
			}
		}
	}
	else
	{
		g_pRegistryMgr->SetNodeIDAsModified(this);
		if(!m_pGlobalSIS)
		{
			m_pGlobalSIS = AkNew(g_DefaultPoolId,CAkSIS( this, GetBypassFX( 0, in_GameObjPtr ) ) ); // FIXME
		}
		l_pSIS = m_pGlobalSIS;
	}
	return l_pSIS;
}
// Returns true if the Context may jump to virtual voices, false otherwise.
AkBelowThresholdBehavior CAkParameterNode::GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior )
{
	if( m_bIsVVoicesOptOverrideParent || Parent() == NULL )
	{
		out_Behavior = (AkVirtualQueueBehavior)m_eVirtualQueueBehavior;
		return (AkBelowThresholdBehavior)m_eBelowThresholdBehavior;
	}
	else
	{
		return static_cast<CAkParameterNode*>( Parent() )->GetVirtualBehavior( out_Behavior );
	}
}

bool CAkParameterNode::Has3DParams()
{
	return m_p3DParameters?true:false;
}

AKRESULT CAkParameterNode::UnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	//Suppose things went wrong
	AKRESULT eResult = CAkParameterNodeBase::UnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );

	// Remove it to not be using it anymore
	
	if( m_p3DParameters && (in_FXID == AK_INVALID_UNIQUE_ID) )
	{
		m_p3DParameters->Lock();
		Gen3DParams * l_p3DParams = m_p3DParameters->GetParams();
		AKASSERT( l_p3DParams != NULL );

		AkReal32 l_Value = 0;
		bool bFound = true;
		switch( in_ParamID )
		{
		case RTPC_Position_PAN_RL:
			l_Value = m_BaseGenParams.m_fPAN_RL;
			break;
		case RTPC_Position_PAN_FR:
			l_Value = m_BaseGenParams.m_fPAN_FR;
			break;
		default:
			bFound = false;
			break;	
		}

		if( bFound )
		{
			PositioningChangeNotification( l_Value, in_ParamID, NULL );
		}
		m_p3DParameters->Unlock();
	}

	return eResult;
}

AKRESULT CAkParameterNode::SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkUInt8 cbPositioningInfoOverrideParent =	READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bPositioningInfoOverrideParent = cbPositioningInfoOverrideParent?true:false;

	if( m_bPositioningInfoOverrideParent )
	{
		//DivergenceCenter
		m_BaseGenParams.m_fCenterPct = READBANKDATA( AkInt, io_rpData, io_rulDataSize ) * 0.01f;

		m_BaseGenParams.m_fPAN_RL = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );
		m_BaseGenParams.m_fPAN_FR = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );

		AkUInt8 cbIs3DPositioningAvailable =	READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		if( !cbIs3DPositioningAvailable )
		{
			//2D
			m_BaseGenParams.bIsPannerEnabled =	( READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) > 0 );
		}
		else
		{
			//3D
			AKASSERT( m_p3DParameters == NULL );

			eResult = Enable3DPosParams();

			if(eResult == AK_Success)
			{
				Gen3DParams * p3DParams = m_p3DParameters->GetParams();

				p3DParams->m_eType = READBANKDATA( AkPositioningType, io_rpData, io_rulDataSize );

				p3DParams->m_uAttenuationID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

				p3DParams->m_bIsSpatialized = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;

				if( p3DParams->m_eType == Ak3DGameDef )
				{
					p3DParams->m_bIsDynamic = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;
				}
				else if( p3DParams->m_eType == Ak3DUserDef )
				{
					p3DParams->m_ePathMode = READBANKDATA( AkPathMode, io_rpData, io_rulDataSize );
					p3DParams->m_bIsLooping = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;
					p3DParams->m_TransitionTime = READBANKDATA( AkTimeMs, io_rpData, io_rulDataSize );

					// Paths
					AkPathVertex * pVertices = NULL;

					AkUInt32 ulNumVertices = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
					if ( ulNumVertices )
					{
						pVertices = (AkPathVertex *) io_rpData;
						SKIPBANKBYTES( sizeof(AkPathVertex) * ulNumVertices, io_rpData, io_rulDataSize );
					}

					AkUInt32 ulNumPlayListItem = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
					if ( ulNumPlayListItem )
					{
						AkPathListItemOffset * pPlayListItems = (AkPathListItemOffset*) io_rpData;
						SKIPBANKBYTES( sizeof( AkPathListItemOffset ) * ulNumPlayListItem, io_rpData, io_rulDataSize );

						if ( ulNumVertices ) 
							eResult = PosSetPath( pVertices, ulNumVertices, pPlayListItems, ulNumPlayListItem );
					}
				}
			}
		}
	}

	return eResult;
}

AKRESULT CAkParameterNode::SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkVirtualQueueBehavior	eVirtualQueueBehavior =		READBANKDATA( AkVirtualQueueBehavior, io_rpData, io_rulDataSize );
	bool bKillNewest =									READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	AkUInt16 u16MaxNumInstances =						READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
	AkBelowThresholdBehavior eBelowThresholdBehavior =	READBANKDATA( AkBelowThresholdBehavior, io_rpData, io_rulDataSize );
	AkUInt8 ucMaxNumInstOverrideParent =				READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 ucVVoicesOptOverrideParent =				READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	SetVirtualQueueBehavior( eVirtualQueueBehavior );
	SetMaxReachedBehavior( bKillNewest );
	SetMaxNumInstances( u16MaxNumInstances );
	SetBelowThresholdBehavior( eBelowThresholdBehavior );
	SetMaxNumInstOverrideParent( ucMaxNumInstOverrideParent != 0 );
	SetVVoicesOptOverrideParent( ucVVoicesOptOverrideParent != 0 );

	return AK_Success;
}

AKRESULT CAkParameterNode::GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj /*= NULL*/
		)
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

		if ( m_pFXChunk )
		{
			out_rFXInfo.EffectTypeID = m_pFXChunk->aFX[ in_uFXIndex ].id;
			out_rFXInfo.pParam = m_pFXChunk->aFX[ in_uFXIndex ].pParam;
			out_rFXInfo.bIsBypassed = GetBypassFX( in_uFXIndex, in_GameObj );
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
	else
	{
		return static_cast<CAkParameterNodeBase *>( Parent() )->GetFX( in_uFXIndex, out_rFXInfo, in_GameObj );
	}
}

bool CAkParameterNode::GetBypassFX(
		AkUInt32	in_uFXIndex,
		CAkRegisteredObj * in_GameObjPtr )
{
	if ( !m_pFXChunk )
		return false;

	bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> in_uFXIndex ) & 1;

	if( m_pFXChunk->aFX[ in_uFXIndex ].id != AK_INVALID_PLUGINID && m_RTPCBitArrayMax32.IsSet( RTPC_BypassFX0 + in_uFXIndex ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this),  RTPC_BypassFX0 + in_uFXIndex, in_GameObjPtr ) != 0;
	}
	else
	{
		CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
		if( l_ppSIS )
		{
			bIsBypass = ( (*l_ppSIS)->m_bitsFXBypass >> in_uFXIndex ) & 1;
		}
		else if( m_pGlobalSIS )
		{
			bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> in_uFXIndex ) & 1;
		}
	}
	return bIsBypass;
}

bool CAkParameterNode::GetBypassAllFX(
		CAkRegisteredObj * in_GameObjPtr )
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		if ( !m_pFXChunk )
			return false;

		bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;

		if( m_RTPCBitArrayMax32.IsSet( RTPC_BypassAllFX ) )
		{
			//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
			bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this),  RTPC_BypassAllFX, in_GameObjPtr ) != 0;
		}
		else
		{
			CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
			if( l_ppSIS )
			{
				bIsBypass = ( (*l_ppSIS)->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
			}
			else if( m_pGlobalSIS )
			{
				bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
			}
		}
		return bIsBypass;
	}
	else
	{
		return static_cast<CAkParameterNodeBase *>( Parent() )->GetBypassAllFX( in_GameObjPtr );
	}
}

void CAkParameterNode::ResetFXBypass(
	AkUInt32		in_bitsFXBypass,
	AkUInt32        in_uTargetMask /* = 0xFFFFFFFF */ )
{
	AKASSERT( !( in_bitsFXBypass & ~in_uTargetMask ) );

	if( m_pGlobalSIS )
	{
		m_pGlobalSIS->m_bitsFXBypass = (AkUInt8) ( ( m_pGlobalSIS->m_bitsFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );
	}
	for( AkMapSIS::Iterator iter = m_mapSIS.Begin(); iter != m_mapSIS.End(); ++iter )
	{
		(*iter).item->m_bitsFXBypass = (AkUInt8) ( ( (*iter).item->m_bitsFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );
	}
}

AkRTPCFXSubscriptionList* CAkParameterNode::GetFxRTPCSubscriptionList()
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		return m_pFXChunk ? &( m_pFXChunk->listFXRTPCSubscriptions ) : NULL;
	}
	else
	{
		return static_cast<CAkParameterNodeBase *>( Parent() )->GetFxRTPCSubscriptionList();
	}
}

AKRESULT CAkParameterNode::AssociateLayer( CAkLayer* in_pLayer )
{
	if ( ! m_associatedLayers.AddLast( in_pLayer ) )
		return AK_InsufficientMemory;

	RecalcNotification();

	return AK_Success;
}

AKRESULT CAkParameterNode::DissociateLayer( CAkLayer* in_pLayer )
{
	AKRESULT eResult = m_associatedLayers.Remove( in_pLayer );

	if ( eResult == AK_Success )
	{
		RecalcNotification();
	}

	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkParameterNode::InvalidatePaths()
{
	//This function is not useless, useful to keep virtual table up when called on destructor
}
#endif
void CAkParameterNode::NotifyRTPCChanged()
{
	if( m_p3DParameters )
	{
		bool l_bIsPanFromRTPC =  m_RTPCBitArrayMax32.IsSet( RTPC_Position_PAN_RL ) || m_RTPCBitArrayMax32.IsSet( RTPC_Position_PAN_FR );
		m_p3DParameters->SetIsPanningFromRTPC( l_bIsPanFromRTPC, m_BaseGenParams );
	}
}


bool CAkParameterNode::IncrementPlayCount( 
	AkPriority in_Priority, 
	CAkRegisteredObj * in_GameObj, 
	AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/,  
	AkUInt16 in_ui16NumKicked /* = 0*/, 
	bool in_bMaxConsidered /*= false*/ 
	)
{
	bool bCanPlayCurrent = true;

	++m_PlayCount;

	if( !in_bMaxConsidered )
	{
		if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
		{
			in_bMaxConsidered = true;
			AkUInt16* pPerObjCount = m_ListPlayCountPerObj.Exists( in_GameObj );
			if( pPerObjCount )
			{
				++(*pPerObjCount);
				if( IsMaxNumInstancesActivated() && ((*pPerObjCount) - in_ui16NumKicked) > GetMaxNumInstances() )
				{
					CAkAudioNode* pKicked = NULL;
					bCanPlayCurrent = CAkURenderer::Kick( in_Priority, in_GameObj, ID(), m_bKillNewest, pKicked );
					// We must increment the kick count only if the kicked one was bound by the same bus restrictions.
					if( !pKicked || pKicked->GetLimitingBus() == GetLimitingBus() )
					{
						++in_ui16NumKicked;
					}
				}
			}
			else
			{
				m_ListPlayCountPerObj.Set( in_GameObj, 1 );
			}
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			bCanPlayCurrent = m_pBusOutputNode->IncrementPlayCount( in_Priority, in_GameObj, AK_ForwardToBusType_ALL, in_ui16NumKicked, false ) && bCanPlayCurrent;
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			bCanPlayCurrent = m_pFeedbackInfo->m_pFeedbackBus->IncrementPlayCount(in_Priority, in_GameObj, AK_ForwardToBusType_ALL, in_ui16NumKicked, false ) && bCanPlayCurrent;
		}
	}

	if(m_pParentNode)
	{
		bCanPlayCurrent = m_pParentNode->IncrementPlayCount( in_Priority, in_GameObj, in_flagForwardToBus, in_ui16NumKicked, in_bMaxConsidered ) && bCanPlayCurrent;
	}

	return bCanPlayCurrent;
}

void CAkParameterNode::DecrementPlayCount( 
	CAkRegisteredObj * in_GameObj, 
	AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/, 
	bool in_bMaxConsidered /*= false*/ 
	)
{
	//Note: in_bMaxConsidered is ignored on parameters node, and the flag is set to false before calling the same function on a bus
	--m_PlayCount;

	if( in_GameObj != NULL )
	{
		if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
		{
			AkUInt16* pPerObjCount = m_ListPlayCountPerObj.Exists( in_GameObj );
			if( pPerObjCount )
			{
				--(*pPerObjCount);
				if( *pPerObjCount == 0 )
				{
					m_ListPlayCountPerObj.Unset( in_GameObj );
				}
			}

			in_GameObj = NULL;
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			m_pBusOutputNode->DecrementPlayCount( in_GameObj, AK_ForwardToBusType_ALL );
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			m_pFeedbackInfo->m_pFeedbackBus->DecrementPlayCount(in_GameObj, AK_ForwardToBusType_ALL );
		}
	}

	if(m_pParentNode)
	{
		m_pParentNode->DecrementPlayCount( in_GameObj, in_flagForwardToBus );
	}
}

bool CAkParameterNode::IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest, bool in_bIsBusChecked, bool in_bIgnoreNonBus )
{
	bool bRet = false;
	if( !in_bIgnoreNonBus )
	{
		bRet = in_NodeIDToTest == ID();
	}
	if( !bRet )
	{
		if( !in_bIsBusChecked && ParentBus() != NULL )
		{
			bRet = static_cast<CAkBus*>( ParentBus() )->IsInstanceCountCompatible( in_NodeIDToTest );
			in_bIsBusChecked = true;
		}
		if( !bRet )
		{
			if( Parent() != NULL )
			{
				if( m_bIsMaxNumInstOverrideParent )
				{
					in_bIgnoreNonBus = true;
				}
				bRet = static_cast<CAkParameterNode*>( Parent() )->IsInstanceCountCompatible( in_NodeIDToTest, in_bIsBusChecked, in_bIgnoreNonBus );
			}
		}
	}
	return bRet;
}

bool CAkParameterNode::IsOrIsChildOf( AkUniqueID in_NodeIDToTest, bool in_bIsBusChecked /*= false*/ )
{
	bool bRet = false;
	bool l_bIsBusChecked = false;
	CAkParameterNode* pNode = this;
	
	while( !bRet && pNode )
	{
		bRet = in_NodeIDToTest == pNode->ID();
		if( !bRet && !l_bIsBusChecked && pNode->ParentBus() != NULL )
		{
			bRet = static_cast<CAkBus*>( ParentBus() )->IsOrIsChildOf( in_NodeIDToTest );
			l_bIsBusChecked = true;
		}
		pNode = static_cast<CAkParameterNode*>( pNode->Parent() );
	}
	return bRet;
}

bool CAkParameterNode::EnableRangeParams()
{
	if(!m_pRangedParams)
	{
		m_pRangedParams = AkNew( g_DefaultPoolId, AkRangedParameters );
	}
	return m_pRangedParams != NULL;
}

AkPathState* CAkParameterNode::GetPathState()
{
	if( m_p3DParameters )
	{
		return &m_PathState;
	}
	else if( Parent() )
	{
		return static_cast<CAkParameterNode*>( Parent() )->GetPathState();
	}
	else
	{
		AKASSERT( !"Path not available" );
		return NULL;
	}
}

AKRESULT CAkParameterNode::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	AKRESULT akr = CAkParameterNodeBase::GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, in_bDoBusCheck);
	if(m_pGlobalSIS)
	{
		io_Params.m_NewVolume += m_pGlobalSIS->m_EffectiveFeedbackVolumeOffset;
	}

	CAkSIS** l_ppSIS = m_mapSIS.Exists( in_GameObjPtr );
	if( l_ppSIS )
	{
		io_Params.m_NewVolume += (**l_ppSIS).m_EffectiveFeedbackVolumeOffset;
	}

	return akr;
}

AkVolumeValue CAkParameterNode::GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr )
{
	AkVolumeValue l_Volume = CAkParameterNodeBase::GetEffectiveFeedbackVolume(in_GameObjPtr);
	
	if(m_pRangedParams)
	{
		l_Volume += RandomizerModifier::GetMod( m_pRangedParams->FeedbackVolume );
	}

	return l_Volume;
}

AkLPFType CAkParameterNode::GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr )
{
	AkLPFType fLPF(CAkParameterNodeBase::GetEffectiveFeedbackLPF(in_GameObjPtr));
	
	if(m_pRangedParams)
	{
		fLPF += RandomizerModifier::GetMod( m_pRangedParams->FeedbackLPF );
	}

	return fLPF;
}
