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
// AkParameterNodeBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkParameterNodeBase.h"
#include "AkTransitionManager.h"
#include "AkStateMgr.h"
#include "AkAudioLibIndex.h"
#include "AkSis.h"
#include "AkMonitor.h"
#include "AkRTPCMgr.h"
#include "AkBitArray.h"
#include "AkBankFloatConversion.h"
#include "AkGen3DParams.h"
#include "AkAudiolib.h"
#include "AkBanks.h"
#include "AkFXMemAlloc.h"
#include "AkEffectsMgr.h"
#include "AkRegisteredObj.h"
#include "AkParameterNode.h"
#include "AkBus.h"
#include "AkFeedbackBus.h"
#include "AkBankMgr.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern AkMemPoolId g_DefaultPoolId;
extern CAkBus * g_pMasterBus;

#define AK_MIN_NUM_STATE_PER_NODE 0
#define AK_MAX_NUM_STATE_PER_NODE 256

CAkParameterNodeBase::FXChunk::FXChunk()
{
	for ( AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
	{
		aFX[i].id = AK_INVALID_PLUGINID;
		aFX[i].pParam = NULL;
		aFX[i].bRendered = false;
	}

	bitsMainFXBypass = 0;
}

CAkParameterNodeBase::FXChunk::~FXChunk()
{
	for( AkRTPCFXSubscriptionList::Iterator iter = listFXRTPCSubscriptions.Begin(); iter != listFXRTPCSubscriptions.End(); ++iter )
		(*iter).ConversionTable.Unset();
	listFXRTPCSubscriptions.Term();

	for ( AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
	{
		if ( aFX[i].pParam )
			aFX[i].pParam->Term( AkFXMemAlloc::GetUpper( ) );
	}
}
		
CAkParameterNodeBase::CAkParameterNodeBase(AkUniqueID in_ulID)
:CAkAudioNode(in_ulID)
,m_ulStateGroup(0)
,m_pGlobalSIS(NULL)
,m_pFXChunk( NULL )
,m_ulActualState(0)
,m_ucPriority(AK_DEFAULT_PRIORITY)
,m_bPriorityApplyDistFactor(false)
,m_iPriorityDistanceOffset( 0 )
,m_bPriorityOverrideParent(false)
,m_pStateTransitionInfo( NULL )
,m_PitchMain( 0 )
,m_LPFMain( 0 )
,m_bUseState( true )
,m_u16MaxNumInstance( 0 ) //0 mans no max
,m_bKillNewest( false )
,m_bIsMaxNumInstOverrideParent( false )
,m_bIsVVoicesOptOverrideParent( false )
,m_eStateSyncType( SyncTypeImmediate )
,m_bIsInDestructor( false )
,m_pFeedbackInfo( NULL ) 
{

}

CAkParameterNodeBase::~CAkParameterNodeBase()
{
	m_bIsInDestructor = true;

	if ( m_pFXChunk )
		AkDelete2( g_DefaultPoolId, FXChunk, m_pFXChunk );

	AKASSERT( g_pRTPCMgr );
	if(g_pRTPCMgr)
	{
		m_RTPCBitArrayMax32.ClearBits();
		g_pRTPCMgr->UnSubscribeRTPC( this );
	}

	if(m_ulStateGroup)
	{
		g_pStateMgr->RemoveStateGroupMember(m_ulStateGroup,this);
	}
	if(m_pGlobalSIS)
	{
		AkDelete(g_DefaultPoolId,m_pGlobalSIS);
	}
	if( m_pStateTransitionInfo )
	{
		FlushStateTransitions();
		AkDelete( g_DefaultPoolId, m_pStateTransitionInfo);
		m_pStateTransitionInfo = NULL;// important to NULL it even in the destructor as it may be checked again in RemoveAllStates() ( and the order must be preserved )
		// m_bIsInDestructor is used to prevent RemoveAllStates to re-create it.
	}
    RemoveAllStates();
	m_mapStates.Term();
	
	if (m_pFeedbackInfo != NULL)
	{
		if (m_pFeedbackInfo->m_pFeedbackBus != NULL)	//Will be null for the master bus
			m_pFeedbackInfo->m_pFeedbackBus->RemoveChild(ID());
		AkDelete2( g_DefaultPoolId, AkFeedbackInfo, m_pFeedbackInfo );
	}
}

void CAkParameterNodeBase::FlushStateTransitions()
{
	if( m_pStateTransitionInfo )
	{
		if( m_pStateTransitionInfo->m_pvVolumeTransition )
		{
			g_pTransitionManager->RemoveTransitionFromList( m_pStateTransitionInfo->m_pvVolumeTransition );
			m_pStateTransitionInfo->m_pvVolumeTransition = NULL;
			DecrementActivityCount();
		}
		if( m_pStateTransitionInfo->m_pvPitchTransition )
		{
			g_pTransitionManager->RemoveTransitionFromList( m_pStateTransitionInfo->m_pvPitchTransition );
			m_pStateTransitionInfo->m_pvPitchTransition = NULL;
			DecrementActivityCount();
		}
		if( m_pStateTransitionInfo->m_pvLPFTransition )
		{
			g_pTransitionManager->RemoveTransitionFromList( m_pStateTransitionInfo->m_pvLPFTransition );
			m_pStateTransitionInfo->m_pvLPFTransition = NULL;
			DecrementActivityCount();
		}
		if( m_pStateTransitionInfo->m_pvLfeTransition )
		{
			g_pTransitionManager->RemoveTransitionFromList( m_pStateTransitionInfo->m_pvLfeTransition );
			m_pStateTransitionInfo->m_pvLfeTransition = NULL;
			DecrementActivityCount();
		}
	}
}

//====================================================================================================
// figure out what we have to pause / resume
//====================================================================================================
void CAkParameterNodeBase::PauseTransitions(bool in_bPause)
{
	AKASSERT(g_pTransitionManager);

	// should we pause ?
	if( m_pStateTransitionInfo )
	{
		if( in_bPause )
		{
			if(m_pStateTransitionInfo->m_pvPitchTransition)
			{
				g_pTransitionManager->Pause(m_pStateTransitionInfo->m_pvPitchTransition);
			}
			if(m_pStateTransitionInfo->m_pvLPFTransition)
			{
				g_pTransitionManager->Pause(m_pStateTransitionInfo->m_pvLPFTransition);
			}
			if(m_pStateTransitionInfo->m_pvVolumeTransition)
			{
				g_pTransitionManager->Pause(m_pStateTransitionInfo->m_pvVolumeTransition);
			}
			if(m_pStateTransitionInfo->m_pvLfeTransition)
			{
				g_pTransitionManager->Pause(m_pStateTransitionInfo->m_pvLfeTransition);
			}
		}
		// we have to resume
		else
		{
			if(m_pStateTransitionInfo->m_pvPitchTransition)
			{
				g_pTransitionManager->Resume(m_pStateTransitionInfo->m_pvPitchTransition);
			}
			if(m_pStateTransitionInfo->m_pvLPFTransition)
			{
				g_pTransitionManager->Resume(m_pStateTransitionInfo->m_pvLPFTransition);
			}
			if(m_pStateTransitionInfo->m_pvVolumeTransition)
			{
				g_pTransitionManager->Resume(m_pStateTransitionInfo->m_pvVolumeTransition);
			}
			if(m_pStateTransitionInfo->m_pvLfeTransition)
			{
				g_pTransitionManager->Resume(m_pStateTransitionInfo->m_pvLfeTransition);
			}
		}
	}
}

void CAkParameterNodeBase::SetTransition(CAkTransition* in_pTransition, AkTransitionTarget in_eTransitionTarget)
{
	if( EnableStateTransitionInfo() )
	{
		if( in_pTransition )
			IncrementActivityCount();

		switch(in_eTransitionTarget)
		{
		case TransitionTarget_Volume:
			AKASSERT( m_pStateTransitionInfo->m_pvVolumeTransition == NULL );
			m_pStateTransitionInfo->m_pvVolumeTransition = in_pTransition;
			break;
		case TransitionTarget_Pitch:
			AKASSERT( m_pStateTransitionInfo->m_pvPitchTransition == NULL );
			m_pStateTransitionInfo->m_pvPitchTransition = in_pTransition;
			break;
		case TransitionTarget_Lfe:
			AKASSERT( m_pStateTransitionInfo->m_pvLfeTransition == NULL );
			m_pStateTransitionInfo->m_pvLfeTransition = in_pTransition;
			break;
		case TransitionTarget_LPF:
			AKASSERT( m_pStateTransitionInfo->m_pvLPFTransition == NULL );
			m_pStateTransitionInfo->m_pvLPFTransition = in_pTransition;
			break;
		default:
			AKASSERT(!"Invalid transition");
		}
	}
}

CAkTransition*  CAkParameterNodeBase::GetTransition(AkTransitionTarget in_eTransitionTarget)
{
	if( EnableStateTransitionInfo() )
	{
		switch( in_eTransitionTarget )
		{
		case TransitionTarget_Volume:
			return m_pStateTransitionInfo->m_pvVolumeTransition;
			break;
		case TransitionTarget_Lfe:
			return m_pStateTransitionInfo->m_pvLfeTransition;
			break;
		case TransitionTarget_Pitch:
			return m_pStateTransitionInfo->m_pvPitchTransition;
			break;
		case TransitionTarget_LPF:
			return m_pStateTransitionInfo->m_pvLPFTransition;
			break;
		default:
			AKASSERT(!"Invalid transition type");
		}
	}
	return NULL;
}

bool CAkParameterNodeBase::SetTransVolume(AkVolumeValue in_Volume)
{
	bool Changed = false;
	if( EnableStateTransitionInfo() )
	{
		Changed = m_pStateTransitionInfo->m_Volume != in_Volume;
		m_pStateTransitionInfo->m_Volume = in_Volume;
	}
	return Changed;
}

bool CAkParameterNodeBase::SetTransPitch(AkPitchValue in_Pitch)
{
	bool Changed = false;
	if( EnableStateTransitionInfo() )
	{
		Changed = m_pStateTransitionInfo->m_Pitch != in_Pitch;
		m_pStateTransitionInfo->m_Pitch = in_Pitch;
	}
	return Changed;
}

bool CAkParameterNodeBase::SetTransLfe(AkVolumeValue in_Lfe)
{
	bool Changed = false;
	if( EnableStateTransitionInfo() )
	{
		Changed = m_pStateTransitionInfo->m_Lfe != in_Lfe;
		m_pStateTransitionInfo->m_Lfe = in_Lfe;
	}
	return Changed;
}

bool CAkParameterNodeBase::SetTransLPF(AkLPFType in_LPF)
{
	bool Changed = false;
	if( EnableStateTransitionInfo() )
	{
		Changed = m_pStateTransitionInfo->m_LPF != in_LPF;
		m_pStateTransitionInfo->m_LPF = in_LPF;
	}
	return Changed;
}

bool CAkParameterNodeBase::DoesChangeMustBeNotified( ParamType in_ParamType )
{
	CAkState* pState = GetState();
	if(pState && UseState())
	{
		AkValueMeaning l_ValMeaning;
		switch( in_ParamType )
		{
		case PT_Volume:
			l_ValMeaning = pState->VolumeMeaning();
			break;
		case PT_Pitch:
			l_ValMeaning = pState->PitchMeaning();
			break;
		case PT_LFE:
			l_ValMeaning = pState->LFEVolumeMeaning();
			break;
		case PT_LPF:
			l_ValMeaning = pState->LPFMeaning();
			break;
		}
		if( l_ValMeaning == AkValueMeaning_Independent )
		{
			return false;
		}
	}
	return true;
}

//====================================================================================================
//====================================================================================================
void CAkParameterNodeBase::StartSisVolumeTransitions(CAkSIS*	in_pSIS,
												 AkReal32		in_fTargetValue,
												 AkValueMeaning	in_eValueMeaning,
												 AkCurveInterpolation	in_eFadeCurve,
												 AkTimeMs		in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);

	if( !in_pSIS )
		return;

	TransitionParameters	VolumeParams;

	// have we got one running already ?
	if(in_pSIS->m_pvVolumeTransition != NULL)
	{
		VolumeParams.uTargetValue.fValue = in_fTargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pSIS->m_pvVolumeTransition,
													TransTarget_Volume,
													VolumeParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning);
	}

	// got none running yet ?
	if(in_pSIS->m_pvVolumeTransition == NULL)
	{
		VolumeParams.uStartValue.fValue = in_pSIS->m_EffectiveVolumeOffset;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Independent:
			VolumeParams.uTargetValue.fValue = in_fTargetValue - m_VolumeMain.GetValue();
			break;
		case AkValueMeaning_Offset:
			VolumeParams.uTargetValue.fValue = in_pSIS->m_EffectiveVolumeOffset + in_fTargetValue;
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
			AkReal32 fNotifValue = in_pSIS->m_EffectiveVolumeOffset;
			in_pSIS->m_EffectiveVolumeOffset = VolumeParams.uTargetValue.fValue;

			VolumeNotification(in_pSIS->m_EffectiveVolumeOffset - fNotifValue, in_pSIS->m_pGameObj);

		}
		// start it
		else
		{
			VolumeParams.eTargetType = AkVolumeFloat;
			VolumeParams.eFadeCurve = in_eFadeCurve;
			VolumeParams.lDuration = in_lTransitionTime;
			VolumeParams.bdBs = true;
			VolumeParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvVolumeTransition = g_pTransitionManager->AddTransitionToList(VolumeParams);
		}
	}
}

//====================================================================================================
//====================================================================================================
void CAkParameterNodeBase::StartSisLFETransitions(CAkSIS*			in_pSIS,
													AkReal32			in_fTargetValue,
													AkValueMeaning	in_eValueMeaning,
													AkCurveInterpolation		in_eFadeCurve,
													AkTimeMs		in_lTransitionTime)
{
	if( !in_pSIS )
		return;

	AKASSERT(g_pTransitionManager);

	TransitionParameters	LFEParams;

	// have we got one running already ?
	if(in_pSIS->m_pvLFETransition != NULL)
	{
		LFEParams.uTargetValue.fValue = in_fTargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pSIS->m_pvLFETransition,
													TransTarget_Lfe,
													LFEParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning);
	}

	// got none running yet ?
	if(in_pSIS->m_pvLFETransition == NULL)
	{
		LFEParams.uStartValue.fValue = in_pSIS->m_EffectiveLFEOffset;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Independent:
			LFEParams.uTargetValue.fValue = in_fTargetValue - m_LFEVolumeMain.GetValue();
			break;
		case AkValueMeaning_Offset:
			LFEParams.uTargetValue.fValue = in_pSIS->m_EffectiveLFEOffset + in_fTargetValue;
			break;
		case AkValueMeaning_Default:
			LFEParams.uTargetValue.fValue = 0.0f;
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((LFEParams.uStartValue.fValue == LFEParams.uTargetValue.fValue)
			|| (in_lTransitionTime == 0))
		{
			// no need to
			AkReal32 fNotifValue = in_pSIS->m_EffectiveLFEOffset;
			in_pSIS->m_EffectiveLFEOffset = LFEParams.uTargetValue.fValue;

			LFENotification(in_pSIS->m_EffectiveLFEOffset - fNotifValue, in_pSIS->m_pGameObj);

		}
		// start it
		else
		{
			LFEParams.eTargetType = AkLfeFloat;
			LFEParams.eFadeCurve = in_eFadeCurve;
			LFEParams.lDuration = in_lTransitionTime;
			LFEParams.bdBs = true;
			LFEParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvLFETransition = g_pTransitionManager->AddTransitionToList( LFEParams );
		}
	}
}

//====================================================================================================
//====================================================================================================
void CAkParameterNodeBase::StartSisLPFTransitions(CAkSIS*		in_pSIS,
												 AkLPFType		in_fTargetValue,
												 AkValueMeaning	in_eValueMeaning,
												 AkCurveInterpolation	in_eFadeCurve,
												 AkTimeMs		in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);

	if( !in_pSIS )
		return;

	TransitionParameters	LPFParams;

	// have we got one running already ?
	if(in_pSIS->m_pvLPFTransition != NULL)
	{
		LPFParams.uTargetValue.fValue = in_fTargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pSIS->m_pvLPFTransition,
													TransTarget_LPF,
													LPFParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning);
	}

	// got none running yet ?
	if(in_pSIS->m_pvLPFTransition == NULL)
	{
		LPFParams.uStartValue.fValue = in_pSIS->m_EffectiveLPFOffset;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Independent:
			LPFParams.uTargetValue.fValue = in_fTargetValue - m_LPFMain;
			break;
		case AkValueMeaning_Offset:
			LPFParams.uTargetValue.fValue = in_pSIS->m_EffectiveLPFOffset + in_fTargetValue;
			break;
		case AkValueMeaning_Default:
			LPFParams.uTargetValue.fValue = 0.0f;
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((LPFParams.uStartValue.fValue == LPFParams.uTargetValue.fValue)
			|| (in_lTransitionTime == 0))
		{
			// no need to
			AkReal32 fNotifValue = in_pSIS->m_EffectiveLPFOffset;
			in_pSIS->m_EffectiveLPFOffset = LPFParams.uTargetValue.fValue;

			LPFNotification(in_pSIS->m_EffectiveLPFOffset - fNotifValue, in_pSIS->m_pGameObj);

		}
		// start it
		else
		{
			LPFParams.eTargetType = AkLPFFloat;
			LPFParams.eFadeCurve = in_eFadeCurve;
			LPFParams.lDuration = in_lTransitionTime;
			LPFParams.bdBs = false;
			LPFParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvLPFTransition = g_pTransitionManager->AddTransitionToList(LPFParams);
		}
	}
}

//====================================================================================================
//====================================================================================================
void CAkParameterNodeBase::StartSisPitchTransitions(CAkSIS*		in_pSIS,
												AkPitchValue		in_TargetValue,
												AkValueMeaning	in_eValueMeaning,
												AkCurveInterpolation		in_eFadeCurve,
												AkTimeMs		in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);
	if( !in_pSIS )
		return;

	TransitionParameters PitchParams;

	// have we got one running already ?
	if( in_pSIS->m_pvPitchTransition != NULL )
	{
		PitchParams.uTargetValue.lValue = in_TargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter( in_pSIS->m_pvPitchTransition,
													TransTarget_Pitch,
													PitchParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning );
	}

	// got none running yet ?
	if( in_pSIS->m_pvPitchTransition == NULL )
	{
		PitchParams.uStartValue.lValue = static_cast<AkInt32>( in_pSIS->m_EffectivePitchOffset );
		switch( in_eValueMeaning )
		{
		case AkValueMeaning_Independent:
			PitchParams.uTargetValue.lValue = in_TargetValue - m_PitchMain;
			break;
		case AkValueMeaning_Offset:
			{
				if(in_TargetValue > 0)
				{
					PitchParams.uTargetValue.lValue = AkMin(in_pSIS->m_EffectivePitchOffset + in_TargetValue, _MAX_PITCH_FOR_NODE);
				}
				else
				{
					PitchParams.uTargetValue.lValue = AkMax(in_pSIS->m_EffectivePitchOffset + in_TargetValue, _MIN_PITCH_FOR_NODE);
				}
			}
			break;
		case AkValueMeaning_Default:
			PitchParams.uTargetValue.lValue = 0;
			break;
		default:
			AKASSERT( !"Invalid Meaning type" );
			break;
		}

		// do we really need to start a transition ?
		if( ( PitchParams.uStartValue.lValue == PitchParams.uTargetValue.lValue )
			|| ( in_lTransitionTime == 0 ) )
		{
			// no need to
			AkPitchValue NotifValue = in_pSIS->m_EffectivePitchOffset;
			in_pSIS->m_EffectivePitchOffset = static_cast<AkPitchValue>( PitchParams.uTargetValue.lValue );

			PitchNotification( in_pSIS->m_EffectivePitchOffset - NotifValue, in_pSIS->m_pGameObj );
		}
		// start it
		else
		{
			PitchParams.eTargetType = AkPitchLong;
			PitchParams.eFadeCurve = in_eFadeCurve;
			PitchParams.lDuration = in_lTransitionTime;
			PitchParams.bdBs = false;
			PitchParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvPitchTransition = g_pTransitionManager->AddTransitionToList( PitchParams );
		}
	}
}

//====================================================================================================
//====================================================================================================
void CAkParameterNodeBase::StartSisMuteTransitions(CAkSIS*	in_pSIS,
													AkUInt8		in_cTargetValue,
													AkCurveInterpolation	in_eFadeCurve,
													AkTimeMs	in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);

	if( !in_pSIS )
		return;

	TransitionParameters	MuteParams;

	// muting or resuming ?
	MuteParams.uStartValue.fValue = ((in_pSIS->m_cMuteLevel - UNMUTED_LVL) * -AK_MINIMUM_VOLUME_LEVEL) / (AkReal32)UNMUTED_LVL;
	if(in_cTargetValue == MUTED_LVL)
	{
		MuteParams.uTargetValue.fValue = AK_MINIMUM_VOLUME_LEVEL;
	}
	else
	{
		MuteParams.uTargetValue.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	}

	// have we got one running already ?
	if(in_pSIS->m_pvMuteTransition != NULL)
	{
		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pSIS->m_pvMuteTransition,
												TransTarget_Mute,
												MuteParams.uTargetValue,
												in_lTransitionTime,
												AkValueMeaning_Default);
	}

	// got none running yet ?
	if(in_pSIS->m_pvMuteTransition == NULL)
	{
		MuteParams.eTargetType = static_cast<TransitionTargetTypes>(TransTarget_Mute | AkTypeFloat);
		

		if( in_lTransitionTime != 0 )
		{
			MuteParams.eFadeCurve = in_eFadeCurve;
			MuteParams.lDuration = in_lTransitionTime;
			MuteParams.bdBs = true;
			MuteParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvMuteTransition = g_pTransitionManager->AddTransitionToList(MuteParams);
		}
		else
		{
			//Apply it directly, so there will be no delay, avoiding an annoying glitch that may apears in the worst cases.
			in_pSIS->TransUpdateValue( MuteParams.eTargetType, MuteParams.uTargetValue, true );
		}
	}
}

void CAkParameterNodeBase::StartSisFeedbackVolumeTransitions(CAkSIS*	in_pSIS,
												 AkReal32		in_fTargetValue,
												 AkValueMeaning	in_eValueMeaning,
												 AkCurveInterpolation	in_eFadeCurve,
												 AkTimeMs		in_lTransitionTime)
{
	AKASSERT(g_pTransitionManager);

	if( !in_pSIS )
		return;

	TransitionParameters	FeedbackVolumeParams;

	// have we got one running already ?
	if(in_pSIS->m_pvFeedbackVolumeTransition != NULL)
	{
		FeedbackVolumeParams.uTargetValue.fValue = in_fTargetValue;

		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pSIS->m_pvFeedbackVolumeTransition,
													TransTarget_FeedbackVolume,
													FeedbackVolumeParams.uTargetValue,
													in_lTransitionTime,
													in_eValueMeaning);
	}

	// got none running yet ?
	if(in_pSIS->m_pvFeedbackVolumeTransition == NULL)
	{
		if (m_pFeedbackInfo == NULL)
		{
			AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo() );
			if (m_pFeedbackInfo == NULL)
				return;
		}

		FeedbackVolumeParams.uStartValue.fValue = in_pSIS->m_EffectiveFeedbackVolumeOffset;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Independent:
			FeedbackVolumeParams.uTargetValue.fValue = in_fTargetValue - m_pFeedbackInfo->m_VolumeFeedback.GetValue();
			break;
		case AkValueMeaning_Offset:
			FeedbackVolumeParams.uTargetValue.fValue = in_pSIS->m_EffectiveFeedbackVolumeOffset + in_fTargetValue;
			break;
		case AkValueMeaning_Default:

			FeedbackVolumeParams.uTargetValue.fValue = 0.0f;
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((FeedbackVolumeParams.uStartValue.fValue == FeedbackVolumeParams.uTargetValue.fValue)
			|| (in_lTransitionTime == 0))
		{
			// no need to
			AkReal32 fNotifValue = in_pSIS->m_EffectiveFeedbackVolumeOffset;
			in_pSIS->m_EffectiveFeedbackVolumeOffset = FeedbackVolumeParams.uTargetValue.fValue;

			FeedbackVolumeNotification(in_pSIS->m_EffectiveFeedbackVolumeOffset - fNotifValue, in_pSIS->m_pGameObj);

		}
		// start it
		else
		{
			FeedbackVolumeParams.eTargetType = AkFeedbackVolumeFloat;
			FeedbackVolumeParams.eFadeCurve = in_eFadeCurve;
			FeedbackVolumeParams.lDuration = in_lTransitionTime;
			FeedbackVolumeParams.bdBs = true;
			FeedbackVolumeParams.pUser = in_pSIS;

			// PhM : AddTransitionToList() will return NULL if none is available
			in_pSIS->m_pvFeedbackVolumeTransition = g_pTransitionManager->AddTransitionToList(FeedbackVolumeParams);
		}
	}
}


AKRESULT CAkParameterNodeBase::SetFX( AkPluginID  	in_FXID,
									  AkUInt32  	in_uFXIndex,
									  void* 		in_pvInitParamsBlock	/*= NULL */, 
									  AkUInt32 		in_ulParamBlockSize	/*= 0 */)
{
	// Check parameters.
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
	{
		return AK_InvalidParameter;
	}

	if ( !m_pFXChunk )
	{
		AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
		if ( !m_pFXChunk )
			return AK_InsufficientMemory;
	}

	FX & fx = m_pFXChunk->aFX[ in_uFXIndex ];

#ifndef AK_OPTIMIZED
	if( fx.id != in_FXID )
		StopMixBus();

	if ( fx.bRendered ) // Do not set an effect if effect has been rendered.
	{
		return AK_RenderedFX;
	}
#endif

	AKRESULT l_eResult = AK_Success;

	// Delete the existing parameter object.
	if( fx.pParam != NULL )
	{
		l_eResult = fx.pParam->Term( AkFXMemAlloc::GetUpper( ) );
		AKASSERT( l_eResult == AK_Success );
		fx.pParam = NULL;
	}

	if( l_eResult == AK_Success )
	{
		IAkPluginParam * l_pEffectParam = NULL;

		// Create and init the parameter object for the specified effect.
		if( fx.pParam == NULL && in_FXID != AK_PLUGINID_ENVIRONMENTAL )
		{	
            AKRESULT l_eResult = CAkEffectsMgr::AllocParams( AkFXMemAlloc::GetUpper(), in_FXID, l_pEffectParam );

			AKASSERT( l_eResult != AK_Success || l_pEffectParam != NULL );
			if( l_eResult != AK_Success || l_pEffectParam == NULL )
			{
				// Yes success. We don't want a bank to fail loading because an FX is not registered or not working.
				// So we don't set the FX and return, as if there was never any FX.
				// The sound will play without FX.
				return AK_Success;
			}

			l_eResult = l_pEffectParam->Init( AkFXMemAlloc::GetUpper(), in_pvInitParamsBlock, in_ulParamBlockSize );
			AKASSERT( l_eResult == AK_Success );
			if( l_eResult != AK_Success )
			{
				l_pEffectParam->Term( AkFXMemAlloc::GetUpper( ) );
				l_pEffectParam = NULL;
				return l_eResult;
			}
		}

		// Add the effect.
		fx.pParam	= l_pEffectParam;
		fx.id		= in_FXID;
	}
	else
	{
		fx.pParam	= NULL;
		fx.id		= AK_INVALID_PLUGINID;
	}

	//Required only so that the volume gets calculated correctly after/before the BUS FX
	RecalcNotification();
	return l_eResult;
}

AKRESULT CAkParameterNodeBase::RemoveFX( AkUInt32 in_uFXIndex )
{
	// Check parameters.
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
	{
		return AK_InvalidParameter;
	}

	if ( !m_pFXChunk )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	StopMixBus();
#endif

	FX & fx = m_pFXChunk->aFX[ in_uFXIndex ];

	AKRESULT l_eResult = AK_Fail;

	// Delete the existing parameter object.
	if( fx.pParam != NULL )
	{
        l_eResult = fx.pParam->Term( AkFXMemAlloc::GetUpper( ) );
		AKASSERT( l_eResult == AK_Success );
	}

	// Remove the effect.
	fx.pParam = NULL;
	fx.id = AK_INVALID_PLUGINID;

	//Required only so that the volume gets calculated correctly after/before the BUS FX
	RecalcNotification();
	return l_eResult;
}

AKRESULT CAkParameterNodeBase::SetFXParam( AkPluginID      	in_FXID,
										   AkUInt32	   		in_uFXIndex,
										   AkPluginParamID 	in_ulParamID,
										   void*     		in_pvParamsBlock,
										   AkUInt32     	in_ulParamBlockSize )
{
	if ( !m_pFXChunk ) 
		return AK_Fail;

	FX & fx = m_pFXChunk->aFX[ in_uFXIndex ];

#ifndef AK_OPTIMIZED
	if ( fx.bRendered ) // Do not set FX Parameter if effect has been rendered.
	{
		return AK_RenderedFX;
	}
#endif

	// Check if there is an effect record for the specified index.
	if( fx.pParam == NULL )
	{
		return AK_InvalidParameter;
	}
	
	AKRESULT l_eResult = fx.pParam->SetParam( in_ulParamID,
											in_pvParamsBlock,
											in_ulParamBlockSize );
// PhM
#ifndef AK_OPTIMIZED
	UpdateFxParam(	in_FXID,
					in_uFXIndex,
					in_ulParamID,
					in_pvParamsBlock,
					in_ulParamBlockSize );
#endif
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
}

AKRESULT CAkParameterNodeBase::RenderedFX( AkUInt32 in_uFXIndex, bool in_bRendered )
{
	AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

	if ( !m_pFXChunk )
	{
		if ( in_bRendered )
		{
			AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
			if ( !m_pFXChunk )
				return AK_InsufficientMemory;
		}
		else
		{
			return AK_Success;
		}
	}

	m_pFXChunk->aFX[ in_uFXIndex ].bRendered = in_bRendered;

	if( in_bRendered && m_pFXChunk->aFX[ in_uFXIndex ].pParam != NULL )
	{
		MONITOR_ERRORMSG2( L"Warning: Bank contains rendered source effects which can't be edited in Wwise", L"" );
		RemoveFX( in_uFXIndex );
	}

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::MainBypassFX( 
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ )
{
	AKASSERT( !( in_bitsFXBypass & ~in_uTargetMask ) );

	if ( !m_pFXChunk )
	{
		if ( in_bitsFXBypass )
		{
			AkNew2( m_pFXChunk, g_DefaultPoolId, FXChunk, FXChunk() );
			if ( !m_pFXChunk )
				return AK_InsufficientMemory;
		}
		else
		{
			return AK_Success;
		}
	}

	m_pFXChunk->bitsMainFXBypass = (AkUInt8) ( ( m_pFXChunk->bitsMainFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );

	if( NodeCategory() == AkNodeCategory_Bus )
	{
		MONITOR_BUSNOTIFICATION(
			ID(), 
			AkMonitorData::BusNotification_FXBypass,
			in_bitsFXBypass, in_uTargetMask	);
	}

	//CheckBox prevail over the rest, should not happen when bound on RTPC since the UI will disable it.
	ResetFXBypass( in_bitsFXBypass, in_uTargetMask );
	
	NotifyBypass( in_bitsFXBypass, in_uTargetMask );

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::BypassFX(
	AkUInt32			in_bitsFXBypass,
	AkUInt32			in_uTargetMask,
	CAkRegisteredObj *	in_pGameObj /* = NULL */,
	bool			in_bIsFromReset /* = false */ )
{
	if( !in_bIsFromReset )
	{//The only reason to not consider it a change is when it is a Reset.
		MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_BypassFXChanged, ID(), in_pGameObj?in_pGameObj->ID():AK_INVALID_GAME_OBJECT );
	}

	CAkSIS* pSIS = GetSIS( in_pGameObj );

	if( pSIS )
	{
		pSIS->m_bitsFXBypass = (AkUInt8) ( ( pSIS->m_bitsFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );
	}

	if( NodeCategory() == AkNodeCategory_Bus && in_pGameObj == NULL )
	{
		MONITOR_BUSNOTIFICATION(
			ID(), 
			AkMonitorData::BusNotification_FXBypass,
			in_bitsFXBypass, in_uTargetMask );
	}
	if( in_pGameObj == NULL )
	{
		ResetFXBypass( in_bitsFXBypass, in_uTargetMask );
	}

	// Notify playing FXs
	NotifyBypass( in_bitsFXBypass, in_uTargetMask, in_pGameObj );

	return AK_Success;
}

AKRESULT CAkParameterNodeBase::ResetBypassFX(
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj /*= NULL*/
		)
{
	return BypassFX( m_pFXChunk ? m_pFXChunk->bitsMainFXBypass : 0, in_uTargetMask, in_pGameObj, true );
}

///////////////////////////////////////////////////////////////////////////
//  STATES
///////////////////////////////////////////////////////////////////////////

void CAkParameterNodeBase::SetStateGroup(AkStateGroupID in_ulStateGroupID)
{
	AKASSERT(g_pStateMgr);
    if (m_ulStateGroup == in_ulStateGroupID )
    {
        return;
    }

	if(m_ulStateGroup)
	{
		g_pStateMgr->RemoveStateGroupMember(m_ulStateGroup,this);
		RemoveAllStates();
		m_ulStateGroup = 0;
	}
	if(in_ulStateGroupID)
	{
		if( g_pStateMgr->AddStateGroupMember(in_ulStateGroupID,this) == AK_Success )
		{
			m_ulStateGroup = in_ulStateGroupID;
			SetActualState(g_pStateMgr->GetState(in_ulStateGroupID));
		}
	}
	NotifyStateParametersModified();
}

void CAkParameterNodeBase::UnsetStateGroup()
{
	AKASSERT(g_pStateMgr);
	if(m_ulStateGroup)
	{
		g_pStateMgr->RemoveStateGroupMember(m_ulStateGroup,this);
		m_ulStateGroup = 0;
	}
	m_ulActualState = 0;
	NotifyStateParametersModified();
}

AkStateGroupID CAkParameterNodeBase::GetStateGroup()
{
	return m_ulStateGroup;
}

CAkState* CAkParameterNodeBase::GetState()
{
	AkStateLink* l_pStateLink = m_mapStates.Exists( m_ulActualState );
	if( l_pStateLink )
	{
		if( ( *l_pStateLink ).bUseStateGroupInfo )
		{
			AKASSERT(g_pStateMgr);
			return g_pStateMgr->GetStatePtr( m_ulStateGroup );
		}
		return ( *l_pStateLink ).pState;
	}
	return NULL;
}

CAkState* CAkParameterNodeBase::GetState(AkStateID in_StateTypeID)
{
	AkStateLink* l_pStateLink = m_mapStates.Exists(in_StateTypeID);
	if( l_pStateLink )
	{
		if( (*l_pStateLink).bUseStateGroupInfo)
		{
			AKASSERT(g_pStateMgr);
			return g_pStateMgr->GetStatePtr(m_ulStateGroup, in_StateTypeID);
		}
		return (*l_pStateLink).pState;
	}
	return NULL;
}

AkStateID CAkParameterNodeBase::ActualState()
{
	return m_ulActualState;
}

void CAkParameterNodeBase::SetActualState(AkStateID in_ulStateID)
{
	m_ulActualState = in_ulStateID;
}

bool CAkParameterNodeBase::UseState() const
{
	return m_bUseState;
}

void CAkParameterNodeBase::UseState(bool in_bUseState)
{
	m_bUseState = in_bUseState;
	NotifyStateParametersModified();
}

AKRESULT CAkParameterNodeBase::LinkStateToStateDefault(AkStateID in_ulStateID)
{
	RemoveState( in_ulStateID );

	AkStateLink l_link;
	l_link.bUseStateGroupInfo = true;
	m_mapStates.Set( in_ulStateID, l_link );
    NotifyStateParametersModified();
	return AK_Success;
}

AKRESULT CAkParameterNodeBase::AddState(AkUniqueID in_ulStateInstanceID, AkStateID in_ulStateID)
{
	AKASSERT(g_pIndex);
	if(!in_ulStateID)
	{
		return AK_InvalidID;
	}
	CAkState* pState = g_pIndex->m_idxCustomStates.GetPtrAndAddRef( in_ulStateInstanceID );

	if(!pState)
	{
		return AK_InvalidInstanceID;
	}

	RemoveState(in_ulStateID);

	AkStateLink Link;
	Link.pState = pState;
	Link.ulStateID = in_ulStateInstanceID;
	Link.bUseStateGroupInfo = false;

	if ( m_mapStates.Set( in_ulStateID, Link ) )
	{
		pState->InitNotificationSystem( this );
		NotifyStateParametersModified();
		return AK_Success;
	}
	else
	{
		pState->Release();
		return AK_Fail;
	}
}

CAkState* CAkParameterNodeBase::State(AkStateID in_ulStateID )
{
	AkStateLink* l_pStateLink = m_mapStates.Exists(in_ulStateID);
	if(!l_pStateLink)
	{
		return NULL;
	}
	else
	{
		return l_pStateLink->pState;
	}
}

AKRESULT CAkParameterNodeBase::RemoveAllStates()
{
	// Do not simply iterate trough the list since RemoveState breaks it on its way, use first
    while( m_mapStates.Length() > 0 )
    {
		RemoveState( m_mapStates.Begin().pItem->key );
    }
    AKRESULT eResult = AK_Success;
    return eResult;
}

AKRESULT CAkParameterNodeBase::RemoveState(AkStateID in_ulStateID)
{
	AKRESULT eResult = AK_Success;
	AkStateLink* l_pStateLink = m_mapStates.Exists(in_ulStateID);
	if( !l_pStateLink )
	{
		eResult = AK_IDNotFound;
	}
	else
	{
		//TODO(alessard) Get rid of the bUseStateGroupInfo flag
		if(!l_pStateLink->bUseStateGroupInfo)
		{
			l_pStateLink->pState->TermNotificationSystem();
			l_pStateLink->pState->Release();
		}
		m_mapStates.Unset( in_ulStateID );
	}
    NotifyStateParametersModified();
	return eResult;
}

void CAkParameterNodeBase::NotifyStateModified()
{
	RecalcNotification();
}

void CAkParameterNodeBase::NotifyStateParametersModified()
{
	AKASSERT(g_pTransitionManager);
	CAkState* pState = GetState();
	TransitionTarget localTarget;
	if( pState && EnableStateTransitionInfo() )
	{
		if( m_pStateTransitionInfo->m_pvVolumeTransition)
		{
			localTarget.fValue = pState->Volume();
			g_pTransitionManager->ChangeParameter(
							m_pStateTransitionInfo->m_pvVolumeTransition,
							TransTarget_Volume,
							localTarget,
							0,
							AkValueMeaning_Default);
		}
		else
		{
			m_pStateTransitionInfo->m_Volume = pState->Volume();
		}

		if(m_pStateTransitionInfo->m_pvPitchTransition)
		{
			localTarget.lValue = pState->Pitch();
			g_pTransitionManager->ChangeParameter(
							m_pStateTransitionInfo->m_pvPitchTransition,
							TransTarget_Pitch,
							localTarget,
							0,
							AkValueMeaning_Default);
		}
		else
		{
			m_pStateTransitionInfo->m_Pitch = pState->Pitch();
		}

		if(m_pStateTransitionInfo->m_pvLPFTransition)
		{
			localTarget.fValue = pState->LPF();
			g_pTransitionManager->ChangeParameter(
							m_pStateTransitionInfo->m_pvLPFTransition,
							TransTarget_LPF,
							localTarget,
							0,
							AkValueMeaning_Default);
		}
		else
		{
			m_pStateTransitionInfo->m_LPF = pState->LPF();
		}

		if(m_pStateTransitionInfo->m_pvLfeTransition)
		{
			localTarget.fValue = pState->LFEVolume();
			g_pTransitionManager->ChangeParameter(
							m_pStateTransitionInfo->m_pvLfeTransition,
							TransTarget_Lfe,
							localTarget,
							0,
							AkValueMeaning_Default);
		}
		else
		{
			m_pStateTransitionInfo->m_Lfe = pState->LFEVolume();
		}
	}
	else if( m_pStateTransitionInfo )
	{
		FlushStateTransitions();

		m_pStateTransitionInfo->m_Volume = 0;
		m_pStateTransitionInfo->m_Pitch = 0;
		m_pStateTransitionInfo->m_LPF = 0;
		m_pStateTransitionInfo->m_Lfe = 0;
	}
	RecalcNotification();
}

void CAkParameterNodeBase::SetMaxReachedBehavior( bool in_bKillNewest )
{
	m_bKillNewest = in_bKillNewest;
}

void CAkParameterNodeBase::SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance )
{
	m_u16MaxNumInstance = in_u16MaxNumInstance;
}

void CAkParameterNodeBase::SetMaxNumInstOverrideParent( bool in_bOverride )
{
	m_bIsMaxNumInstOverrideParent = in_bOverride;
}

void CAkParameterNodeBase::SetVVoicesOptOverrideParent( bool in_bOverride )
{
	m_bIsVVoicesOptOverrideParent = in_bOverride;
}

AkPriority CAkParameterNodeBase::GetPriority( AkPriority& out_iDistOffset )
{
	if( Parent() && !m_bPriorityOverrideParent)
	{
		return static_cast<CAkParameterNodeBase*>(Parent())->GetPriority( out_iDistOffset );
	}
	else
	{
		out_iDistOffset = m_bPriorityApplyDistFactor ? m_iPriorityDistanceOffset : 0;
		return m_ucPriority;
	}
}

void CAkParameterNodeBase::SetPriority( AkPriority in_ucPriority )
{
	AKASSERT(in_ucPriority <= AK_MAX_PRIORITY);
	m_ucPriority = in_ucPriority;
}

void CAkParameterNodeBase::SetPriorityApplyDistFactor( bool in_bApplyDistFactor )
{
	m_bPriorityApplyDistFactor = in_bApplyDistFactor;
}

void CAkParameterNodeBase::SetPriorityDistanceOffset( AkPriority in_iDistOffset )
{
	m_iPriorityDistanceOffset = in_iDistOffset;
}

void CAkParameterNodeBase::SetPriorityOverrideParent( bool in_bOverrideParent )
{
	m_bPriorityOverrideParent = in_bOverrideParent;
}

void CAkParameterNodeBase::RecalcNotification()
{

}

//====================================================================================================
// this is where transitions report changes
//====================================================================================================
void CAkParameterNodeBase::TransUpdateValue(TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated)
{
	CAkTransition**	ThisTransition;

	AKASSERT( m_pStateTransitionInfo );

	// it's gotta be defined
	AKASSERT((in_eTargetType & TransTarget_TargetMask) != TransTarget_UndefinedTarget);

	switch(in_eTargetType & TransTarget_TargetMask)
	{
	case TransTarget_Volume:
		m_pStateTransitionInfo->m_Volume = in_unionValue.fValue;
		ThisTransition = &m_pStateTransitionInfo->m_pvVolumeTransition;
		break;

	case TransTarget_Pitch:
		m_pStateTransitionInfo->m_Pitch = static_cast<AkPitchValue>(in_unionValue.lValue);
		ThisTransition = &m_pStateTransitionInfo->m_pvPitchTransition;
		break;

	case TransTarget_LPF:
		m_pStateTransitionInfo->m_LPF = static_cast<AkLPFType>(in_unionValue.fValue);
		ThisTransition = &m_pStateTransitionInfo->m_pvLPFTransition;
		break;

	case TransTarget_Lfe:
		m_pStateTransitionInfo->m_Lfe = in_unionValue.fValue;
		ThisTransition = &m_pStateTransitionInfo->m_pvLfeTransition;
		break;

	case TransTarget_PlayStop:
	case TransTarget_PauseResume:
	case TransTarget_Mute:
		AKASSERT(!"Unknown transition type");
		break;

	default:
		AKASSERT(0);
		break;
	}

	// if transition is done clear the pointer
	if(in_bIsTerminated)
	{
		if( *ThisTransition != NULL )
		{
			DecrementActivityCount();
			*ThisTransition = NULL;
		}
	}

	RecalcNotification();
}

AKRESULT CAkParameterNodeBase::SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKASSERT(!"Dummy  virtual function");
	return AK_NotImplemented;
}

AKRESULT CAkParameterNodeBase::SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKASSERT(!"Dummy  virtual function");
	return AK_NotImplemented;
}

AKRESULT CAkParameterNodeBase::SetNodeBaseParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly )
{
	AKRESULT eResult = AK_Success;

	//Read FX
	eResult = SetInitialFxParams(io_rpData, io_rulDataSize, in_bPartialLoadOnly);
	if( eResult != AK_Success || in_bPartialLoadOnly )
	{
		return eResult;
	}

	AkUniqueID OverrideBusId = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

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
			// It is now an error to not load the bank content in the proper order.
			MONITOR_ERRORMSG2( L"Master bus structure not loaded: ", L"Make sure that the first bank to be loaded contains the master bus information" );
			eResult = AK_Fail;
		}

		if( eResult != AK_Success )
		{	
			return eResult;
		}
	}

	AkUniqueID DirectParentID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	if( DirectParentID != AK_INVALID_UNIQUE_ID )
	{
		CAkAudioNode* pParent = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( DirectParentID );
		if( pParent )
		{
			eResult = pParent->AddChild( ID() );
			pParent->Release();
			if( eResult != AK_Success )
			{	
				return eResult;
			}
		}
	}

	AkUInt8 ucPriority = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 bPriorityOverrideParent = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 bPriorityApplyDistFactor = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkInt8 iPriorityDistanceOffset = READBANKDATA( AkInt8, io_rpData, io_rulDataSize );

	SetPriority( ucPriority );
	SetPriorityOverrideParent( bPriorityOverrideParent != 0 );
	SetPriorityApplyDistFactor( bPriorityApplyDistFactor != 0 );
	SetPriorityDistanceOffset( iPriorityDistanceOffset );

	if(eResult == AK_Success)
	{
		// Read Initial params
		eResult = SetInitialParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		// Read Initial params
		eResult = SetPositioningParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		// Read Initial params
		eResult = SetAdvSettingsParams( io_rpData, io_rulDataSize );
	}

	if(eResult == AK_Success)
	{
		SetStateSyncType( READBANKDATA( AkUInt32, io_rpData, io_rulDataSize ) );

		// Read Num States
		AkUInt32 ulNumStates = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

		for( AkUInt32 i = 0 ; i < ulNumStates ; ++i )
		{
			AkBank::AKBKStateItem item = READBANKDATA(AkBank::AKBKStateItem, io_rpData, io_rulDataSize);

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
		eResult = SetInitialRTPC(io_rpData, io_rulDataSize);
	}

	//Read feedback info
	if (eResult == AK_Success)
	{
		ReadFeedbackInfo(io_rpData, io_rulDataSize);
	}

	return eResult;
}

void CAkParameterNodeBase::ReadFeedbackInfo(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize)
{
	if (!CAkBankMgr::BankHasFeedback())
		return;

	// TODO : mjean: review this code, now we only have one 1 feedback bus per object
	if (m_pFeedbackInfo == NULL)
	{
		AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
		if (m_pFeedbackInfo == NULL)
			return;
	}

	AkUniqueID BusId = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	if(BusId != 0)
	{
		CAkAudioNode* pBus = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( BusId );
		if(pBus)
		{
			pBus->AddChild( ID() );
			pBus->Release();
		}
		
		//Feedback volume
		AkVolumeValue fFeedbackVolume = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
		AkVolumeValue fFeedbackModifierMin = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
		AkVolumeValue fFeedbackModifierMax = READBANKDATA( AkVolumeValue, io_rpData, io_rulDataSize );
		FeedbackVolume( fFeedbackVolume, fFeedbackModifierMin, fFeedbackModifierMax);

		//Feedback LPF
		AkLPFType fFeedbackLPF = static_cast<AkLPFType>(READBANKDATA( AkLPFType, io_rpData, io_rulDataSize ));
		AkLPFType fFeedbackLPFModMin = READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );
		AkLPFType fFeedbackLPFModMax = READBANKDATA( AkLPFType, io_rpData, io_rulDataSize );
		FeedbackLPF(fFeedbackLPF, fFeedbackLPFModMin, fFeedbackLPFModMax);
	}
}

AKRESULT CAkParameterNodeBase::SetInitialRTPC(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	// Read Num RTPC
	AkUInt32 ulNumRTPC = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

	for(AkUInt32 i = 0; i < ulNumRTPC; ++i)
	{
		AkPluginID l_FXID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
		// Read RTPCID
		AkRtpcID l_RTPCID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read ParameterID//ie. Volume, Pitch, LFE...
		AkRTPC_ParameterID l_ParamID = (AkRTPC_ParameterID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read Curve ID
		const AkUniqueID rtpcCurveID = (AkUniqueID)READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		// Read curve scaling type (None, dB...)
		AkCurveScaling eScaling = READBANKDATA(AkCurveScaling, io_rpData, io_rulDataSize);
		// ArraySize //i.e.:number of conversion points
		AkUInt32 ulSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

		SetRTPC( l_FXID, l_RTPCID, l_ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)io_rpData, ulSize );

		//Skipping the Conversion array
		io_rpData += ( ulSize*sizeof(AkRTPCGraphPoint) );
		io_rulDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
	}

	return AK_Success;
}

// IMPORTANT: modify SetParamComplex along with this
AKRESULT CAkParameterNodeBase::SetParamComplexFromRTPCManager( 
		void * in_pToken,
		AkUInt32 in_Param_id, 
		AkRtpcID in_RTPCid,
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj,
		void* in_pGameObjExceptArray
		)
{
	AKASSERT( m_RTPCBitArrayMax32.IsSet( in_Param_id ) );

	GameObjExceptArray* l_pExcept = static_cast<GameObjExceptArray*>(in_pGameObjExceptArray);
	AKRESULT eResult = AK_Success;

	AkReal32 l_fromValue = 0;
	switch(in_Param_id)
	{
	case RTPC_Volume:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		VolumeNotification( in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_LFE:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		LFENotification( in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_Pitch:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		// Must cast both from and to value before doing the difference, to avoid loosing precisions.
		PitchNotification( ( static_cast<AkPitchValue>(in_value) - static_cast<AkPitchValue>(l_fromValue) ), in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_LPF:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		LPFNotification( ( in_value - l_fromValue ), in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX0:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 0, 1 << 0, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX1:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 1, 1 << 1, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX2:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 2, 1 << 2, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_BypassFX3:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << 3, 1 << 3, in_GameObj, in_pGameObjExceptArray );
		break;
	
	case RTPC_BypassAllFX:
		NotifyBypass( ( ( in_value != 0 ) ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_FeedbackVolume:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		FeedbackVolumeNotification( in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );
		break;

	case RTPC_FeedbackLowpass:
		l_fromValue = g_pRTPCMgr->GetRTPCConvertedValue( in_pToken, in_GameObj, in_RTPCid  );
		FeedbackLPFNotification( in_value - l_fromValue, in_GameObj, in_pGameObjExceptArray );
		break;

	case POSID_Positioning_Cone_LPF:
	case POSID_Positioning_Divergence_Center_PCT:
	case POSID_Positioning_Cone_Attenuation_ON_OFF:
	case POSID_Positioning_Cone_Attenuation:
	
	case POSID_Position_PAN_RL:
	case POSID_Position_PAN_FR:
	case POSID_PositioningType:
	case POSID_ConeInsideAngle:
	case POSID_ConeOutsideAngle:
	case POSID_IsPositionDynamic:
	case POSID_IsLooping:
	case POSID_Transition:
	case POSID_PathMode:
		PositioningChangeNotification( in_value, (AkRTPC_ParameterID)in_Param_id, in_GameObj, in_pGameObjExceptArray );
		break;

	default:
		AKASSERT( !"Receiving an unexpected RTPC notification, ignoring the unknown notification" );
		eResult = AK_Fail;
		break;
	}

	return eResult;
}

// IMPORTANT: modify SetParamComplexFromRTPCManager along with this
AKRESULT CAkParameterNodeBase::SetParamComplex( 
		AkUInt32 in_Param_id, 
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj,
		void* in_pGameObjExceptArray
		)
{
	AKRESULT eResult = AK_Success;

	switch(in_Param_id)
	{
	case POSID_Positioning_Divergence_Center_PCT:
	case POSID_Positioning_Cone_Attenuation_ON_OFF:
	case POSID_Positioning_Cone_Attenuation:
	case POSID_Positioning_Cone_LPF:
	case POSID_Position_PAN_RL:
	case POSID_Position_PAN_FR:

	case POSID_PositioningType:
	case POSID_2DPannerEnabled:
	case POSID_ConeInsideAngle:
	case POSID_ConeOutsideAngle:
	case POSID_IsPositionDynamic:
	case POSID_IsLooping:
	case POSID_Transition:
	case POSID_PathMode:
		PositioningChangeNotification( in_value, (AkRTPC_ParameterID)in_Param_id, in_GameObj, in_pGameObjExceptArray );
		break;

	default:
		AKASSERT( !"Receiving an unexpected RTPC notification, ignoring the unknown notification" );
		eResult = AK_Fail;
		break;
	}

	return eResult;
}

void CAkParameterNodeBase::SetRTPC(
		AkPluginID					in_FXID,		// If invalid, means that the RTPC is directly on sound parameters
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,		// NULL if none
		AkUInt32						in_ulConversionArraySize	// 0 if none
		)
{
	AKASSERT(g_pRTPCMgr);

	if( in_FXID == AK_INVALID_UNIQUE_ID )//i.e. : if this is a core parameter, not an FX or source parameter
	{
		AKASSERT( in_ParamID < MAX_BITFIELD_NUM );

		m_RTPCBitArrayMax32.SetBit( in_ParamID );

		if( g_pRTPCMgr )
		{
			g_pRTPCMgr->SubscribeRTPC( 
				this,
				in_RTPC_ID, 
				in_ParamID, 
				in_RTPCCurveID,
				in_eScaling,
				in_pArrayConversion, 
				in_ulConversionArraySize,
				NULL,
				GetRTPCSubscriberType()
				);
		}

		NotifyRTPCChanged();
	
	}
	else
	{
		SetRTPCforFX(in_FXID, in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
}

AKRESULT CAkParameterNodeBase::UnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	//Suppose things went wrong
	AKRESULT eResult = AK_Success;

	AKASSERT(g_pRTPCMgr);

	if( in_FXID == AK_INVALID_UNIQUE_ID )//i.e. : if this is a core parameter, not an FX or source parameter
	{
		AKASSERT( in_ParamID < MAX_BITFIELD_NUM );

		bool bMoreCurvesRemaining = false;

		if( g_pRTPCMgr )
		{
			eResult = g_pRTPCMgr->UnSubscribeRTPC(
				this,
				in_ParamID,
				in_RTPCCurveID,
				&bMoreCurvesRemaining
				);
		}

		if ( ! bMoreCurvesRemaining )
			m_RTPCBitArrayMax32.UnsetBit( in_ParamID );

		RecalcNotification();

		NotifyRTPCChanged();
	}
	else
	{
		eResult = UnsetRTPCforFX( in_FXID, in_ParamID, in_RTPCCurveID );
	}

	return eResult;
}

AKRESULT CAkParameterNodeBase::SetRTPCforFX(
		AkPluginID					in_FXID,
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32					in_ulConversionArraySize
		)
{
	// AKASSERT( m_pFXChunk ); when an effect is platform-excluded, its RTPC still exist on that platform
	if ( !m_pFXChunk )
		return AK_Fail;

	AKRESULT eResult = AK_Success;

	AKASSERT( in_FXID != AK_INVALID_UNIQUE_ID );
	UnsetRTPCforFX( in_FXID, in_ParamID, in_RTPCCurveID );

	AkRTPCFXSubscription * pSubsItem = m_pFXChunk->listFXRTPCSubscriptions.AddLast();
	if ( pSubsItem )
	{
		pSubsItem->FXID = in_FXID;
		pSubsItem->ParamID = in_ParamID;
		pSubsItem->RTPCCurveID = in_RTPCCurveID;
		pSubsItem->RTPCID = in_RTPC_ID;

		if( in_pArrayConversion && in_ulConversionArraySize )
		{
			eResult = pSubsItem->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );
		}

		UpdateRTPC( *pSubsItem );
	}
	else
	{
		eResult = AK_Fail;
	}


	return eResult;
}

AKRESULT CAkParameterNodeBase::UnsetRTPCforFX(
	AkPluginID in_FXID,
	AkRTPC_ParameterID	in_ParamID,
	AkUniqueID in_RTPCCurveID
	)
{
	if ( !m_pFXChunk )
		return AK_Success;

	AkRTPCFXSubscriptionList::Iterator iter = m_pFXChunk->listFXRTPCSubscriptions.Begin();
	while( iter != m_pFXChunk->listFXRTPCSubscriptions.End() )
	{
		if( (*iter).FXID == in_FXID && (*iter).ParamID == in_ParamID && (*iter).RTPCCurveID == in_RTPCCurveID )
		{
			(*iter).ConversionTable.Unset();
			iter = m_pFXChunk->listFXRTPCSubscriptions.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	NotifUnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );

	return AK_Success;
}

bool CAkParameterNodeBase::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes, bool in_bBusChecked /*=false*/ )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;

	if( !in_bBusChecked && ParentBus() )
	{
		in_bBusChecked = true;
		if( static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	if( Parent() )
	{
		return static_cast<CAkParameterNodeBase*>(Parent())->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes, in_bBusChecked );
	}
	return false;
}

bool CAkParameterNodeBase::CheckSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	if( in_stateGroupID == m_ulStateGroup )
	{
		if( GetStateSyncType() == SyncTypeImmediate )
		{
			io_pSyncTypes->RemoveAllSync();
            io_pSyncTypes->GetStateSyncArray().AddLast( GetStateSyncType() );
			return true;
		}
		else
		{
			bool bFound = false;
			for( StateSyncArray::Iterator iter = io_pSyncTypes->GetStateSyncArray().Begin(); iter != io_pSyncTypes->GetStateSyncArray().End(); ++iter )
			{
				if( *iter == GetStateSyncType() )
				{
					bFound = true;
					break;
				}
			}
			if( !bFound )
			{
				io_pSyncTypes->GetStateSyncArray().AddLast( GetStateSyncType() );
			}
		}
	}
	return false;
}


void CAkParameterNodeBase::NotifyRTPCChanged()
{
	// No IMP at this level, the bus does not have positionning, so no positionning on RTPC
}

bool CAkParameterNodeBase::EnableStateTransitionInfo()
{
	if( !m_pStateTransitionInfo && !m_bIsInDestructor )
	{
		m_pStateTransitionInfo = AkNew( g_DefaultPoolId, AkStateTransitionInfo );
	}
	return m_pStateTransitionInfo != NULL;
}


CAkRTPCMgr::SubscriberType CAkParameterNodeBase::GetRTPCSubscriberType() const
{
	return CAkRTPCMgr::SubscriberType_CAkParameterNodeBase;
}

// Set the feedback volume
void CAkParameterNodeBase::SetFeedbackVolume(
	CAkRegisteredObj *	in_GameObjPtr,				//Game object associated to the action
	AkValueMeaning	in_eValueMeaning,		//Target value meaning
	AkReal32			in_fTargetValue,	// Volume target value
	AkCurveInterpolation in_eFadeCurve ,
	AkTimeMs		in_lTransitionTime)
{

#ifndef AK_OPTIMIZED
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_FeedbackVolumeChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

	AkCompactedVolume FeedbackVolumeNew;
	FeedbackVolumeNew.SetValue( in_fTargetValue );
	if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
		|| ( in_eValueMeaning == AkValueMeaning_Independent && in_fTargetValue != FeedbackVolume() ) )
	{
		MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_FeedbackVolumeChanged, ID(), in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
	}
#endif

	StartSisFeedbackVolumeTransitions( GetSIS( in_GameObjPtr ), in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

// Sets the feedback volume
void CAkParameterNodeBase::FeedbackVolume(AkVolumeValue in_Volume, AkVolumeValue in_MinRangeValue/*=0.0f*/, AkVolumeValue in_MaxRangeValue/*=0.0f*/)
{
	if (m_pFeedbackInfo == NULL)
	{
		if (in_Volume == 0.0)
			return;	//Don't allocate if the default value hasn't changed.

		AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
		if (m_pFeedbackInfo == NULL)
			return;
	}

	AkVolumeValue fOldVolume = m_pFeedbackInfo->m_VolumeFeedback.GetValue();
	m_pFeedbackInfo->m_VolumeFeedback.SetValue(in_Volume);

	FeedbackVolumeNotification( in_Volume - fOldVolume );
}

AkVolumeValue CAkParameterNodeBase::FeedbackVolume()
{
	if (m_pFeedbackInfo == NULL)
		return 0.0;

	return m_pFeedbackInfo->m_VolumeFeedback.GetValue();
}

// Sets the feedback low pass filter
void CAkParameterNodeBase::FeedbackLPF( AkLPFType in_FeedbackLPF, AkLPFType in_MinRangeValue/*=0*/, AkLPFType in_MaxRangeValue/*=0*/)
{
	if (!m_pFeedbackInfo)
	{
		AkNew2(m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo());
		if (!m_pFeedbackInfo)
			return;	//out off memory
	}

	AkLPFType fOldLPF = m_pFeedbackInfo->m_FeedbackLPF;
	m_pFeedbackInfo->m_FeedbackLPF = in_FeedbackLPF;	
	FeedbackLPFNotification ( in_FeedbackLPF - fOldLPF);
}



// Set the output bus for a specific feedback device
void CAkParameterNodeBase::FeedbackParentBus(CAkFeedbackBus* in_pParent)
{
	// Avoid creating the structure if we are not connected.
	if (in_pParent == NULL && m_pFeedbackInfo == NULL)
		return;

	if (m_pFeedbackInfo == NULL)
	{
		AkNew2( m_pFeedbackInfo, g_DefaultPoolId, AkFeedbackInfo, AkFeedbackInfo() );
		if (m_pFeedbackInfo == NULL)
			return;
	}

	m_pFeedbackInfo->m_pFeedbackBus = in_pParent;
}

CAkFeedbackBus* CAkParameterNodeBase::FeedbackParentBus()
{
	if (m_pFeedbackInfo == NULL)
		return NULL;

	return m_pFeedbackInfo->m_pFeedbackBus;
}

CAkFeedbackBus* CAkParameterNodeBase::GetFeedbackParentBusOrDefault()
{
	CAkFeedbackBus* pParent = FeedbackParentBus();

	if( !pParent )
	{
		// linked with: WG-10155 and WG-9702
		// Hack : fixing a whole bunch of bugs with this hack.
		// Actually seems bullet proof, but the user may not have realized that in the project at this point and maybe
		// outputting directy in the motion bus is not what the user wanted.
		pParent = CAkFeedbackBus::GetMasterBus();
		pParent->AddChild( ID() );// ignoring error code, it simply has to work.
	}
	
	AKASSERT( pParent );
	return pParent;
}

// Get the compounded feedback parameters.  There is currenly only the volume.
AKRESULT CAkParameterNodeBase::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	bool bFirst = io_Params.m_usPluginID == AkFeedbackParams::UNINITIALIZED;

	if (m_pFeedbackInfo != NULL)
	{
		// Get the volume of this node
		AkVolumeValue l_Volume = GetEffectiveFeedbackVolume(in_GameObjPtr);
		AkLPFType fLPF = GetEffectiveFeedbackLPF(in_GameObjPtr);

		// The bus check is done only once, for the first object (the leaf in the tree).
		// We need to prime the structure with the first volume and the busses		
		if(in_bDoBusCheck)
		{
			CAkFeedbackBus* pBus = m_pFeedbackInfo->m_pFeedbackBus;
			if (bFirst)
			{
				//The param structure isn't initialized yet.  It is the first time
				//we compute the volume on this object.
				io_Params.m_pOutput = pBus;					
				io_Params.m_NewVolume = l_Volume;
				io_Params.m_LPF = fLPF;
				io_Params.m_usPluginID = AkFeedbackParams::ALL_DEVICES;
			}
			else
			{
				//This is an update of the parameters.  Keep the old values.
				io_Params.m_NewVolume = l_Volume;
				io_Params.m_LPF = fLPF;
			}

			//Walk up the feedback busses
			pBus->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);

			//Remove the audio bus volume, up until we see an effect.  This the point where 
			//the bus volumes stop being collapsed into the source.
			CAkParameterNodeBase* pAudioBus = static_cast<CAkParameterNodeBase*>(m_pBusOutputNode);
			while(pAudioBus != NULL)
			{
				bool bEffect = false;
				if ( pAudioBus->m_pFXChunk )
				{
					for(AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; i++)
						bEffect |= (pAudioBus->m_pFXChunk->aFX[i].id != AK_INVALID_PLUGINID);
				}

				if (bEffect)
					break;
				else
					io_Params.m_AudioBusVolume += pAudioBus->m_VolumeMain.GetValue();

				pAudioBus = static_cast<CAkParameterNodeBase*>(pAudioBus->m_pBusOutputNode);
			}
		}
		else
		{
			// Apply the volume of this node.  It is not related to a particular bus, so
			// we simply apply to all.
			io_Params.m_NewVolume += l_Volume;
			io_Params.m_LPF += fLPF;
		}
	}

	// Get the parent's volumes
	if(m_pParentNode != NULL)
		static_cast<CAkParameterNodeBase*>(m_pParentNode)->GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, false);	

	return AK_Success;
}

AkVolumeValue CAkParameterNodeBase::GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr )
{
	AkVolumeValue l_Volume;
	if (m_pFeedbackInfo != NULL)
		l_Volume = m_pFeedbackInfo->m_VolumeFeedback.GetValue();

	if( m_RTPCBitArrayMax32.IsSet( RTPC_FeedbackVolume ) )
		l_Volume += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_FeedbackVolume, in_GameObjPtr );

	return l_Volume;
}

AkLPFType CAkParameterNodeBase::GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr )
{
	AkLPFType fLPF(0.f);
	if (m_pFeedbackInfo != NULL)
		fLPF = m_pFeedbackInfo->m_FeedbackLPF;

	if( m_RTPCBitArrayMax32.IsSet( RTPC_FeedbackLowpass ) )
		fLPF += g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_FeedbackLowpass, in_GameObjPtr );

	return fLPF;
}

void CAkParameterNodeBase::IncrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	++m_uActivityCount;
	AKASSERT( m_uActivityCount );//wrapped around, this will not cause bad damage, but unrequired notifications car be triggered.

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if( m_pBusOutputNode )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			m_pBusOutputNode->IncrementActivityCount();
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			m_pFeedbackInfo->m_pFeedbackBus->IncrementActivityCount();
		}
	}

	if( m_pParentNode )
	{
		m_pParentNode->IncrementActivityCount( in_flagForwardToBus );
	}
}

void CAkParameterNodeBase::DecrementActivityCount( AkUInt16 in_flagForwardToBus /*= AK_ForwardToBusType_ALL*/ )
{
	AKASSERT( m_uActivityCount );// we had more decrement than increment, not normal.
	--m_uActivityCount;

	if( in_flagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if( m_pBusOutputNode )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Normal;
			m_pBusOutputNode->DecrementActivityCount();
		}
	}

	if( in_flagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			in_flagForwardToBus &= ~AK_ForwardToBusType_Motion;
			m_pFeedbackInfo->m_pFeedbackBus->DecrementActivityCount();
		}
	}

	if( m_pParentNode )
	{
		m_pParentNode->DecrementActivityCount( in_flagForwardToBus );
	}
}
