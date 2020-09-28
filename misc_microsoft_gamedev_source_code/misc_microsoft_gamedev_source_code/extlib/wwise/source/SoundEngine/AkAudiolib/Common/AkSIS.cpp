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
// AkSIS.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkSIS.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkParameterNode.h"
#include "AkMonitor.h"
#include "AkMutedMap.h"
#include "AkRegisteredObj.h"

CAkSIS::CAkSIS( CAkParameterNodeBase* in_Parent, AkUInt8 in_bitsFXBypass )
:m_EffectivePitchOffset(0)
,m_EffectiveVolumeOffset(0)
,m_EffectiveLFEOffset(0)
,m_EffectiveLPFOffset(0)
,m_EffectiveFeedbackVolumeOffset(0)
,m_bitsFXBypass(in_bitsFXBypass)
,m_cMuteLevel( UNMUTED_LVL )
,m_pvVolumeTransition( NULL )
,m_pvLFETransition( NULL )
,m_pvLPFTransition( NULL )
,m_pvPitchTransition( NULL )
,m_pvMuteTransition( NULL )
,m_pvFeedbackVolumeTransition( NULL )
,m_pParamNode( in_Parent )
,m_pGameObj( NULL )
{
}

CAkSIS::CAkSIS( CAkParameterNodeBase* in_Parent, CAkRegisteredObj * in_pGameObj, AkUInt8 in_bitsFXBypass )
:m_EffectivePitchOffset(0)
,m_EffectiveVolumeOffset(0)
,m_EffectiveLFEOffset(0)
,m_EffectiveLPFOffset(0)
,m_EffectiveFeedbackVolumeOffset(0)
,m_bitsFXBypass(in_bitsFXBypass)
,m_cMuteLevel( UNMUTED_LVL )
,m_pvVolumeTransition( NULL )
,m_pvLFETransition( NULL )
,m_pvLPFTransition( NULL )
,m_pvPitchTransition( NULL )
,m_pvMuteTransition( NULL )
,m_pvFeedbackVolumeTransition( NULL )
,m_pParamNode( in_Parent )
,m_pGameObj( in_pGameObj )
{
}

CAkSIS::~CAkSIS()
{
	AKASSERT(g_pTransitionManager);
	if(m_pvVolumeTransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvVolumeTransition);
		m_pvVolumeTransition = NULL;
	}
	if(m_pvLFETransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvLFETransition);
		m_pvLFETransition = NULL;
	}
	if(m_pvLPFTransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvLPFTransition);
		m_pvLPFTransition = NULL;
	}
	if(m_pvPitchTransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvPitchTransition);
		m_pvPitchTransition = NULL;
	}
	if(m_pvMuteTransition)
	{
		g_pTransitionManager->RemoveTransitionFromList(m_pvMuteTransition);
		m_pvMuteTransition = NULL;
	}
}

void CAkSIS::TransUpdateValue( TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated )
{
	AkReal32 fNotified;
	AkPitchValue Notified;

	switch( in_eTargetType & TransTarget_TargetMask )
	{
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_Pitch:
		{
			AKASSERT(( in_eTargetType & AkTypeMask ) == AkTypeLong);
			if(in_bIsTerminated)
			{
				m_pvPitchTransition = NULL;
			}
			AkPitchValue NewValue = in_unionValue.lValue;
			if( NewValue > 0 )
			{
				NewValue = AkMin( NewValue, _MAX_PITCH_FOR_NODE );
			}
			else
			{
				NewValue = AkMax( NewValue, _MIN_PITCH_FOR_NODE );
			}

			// compute the value to be notified
			Notified = NewValue - m_EffectivePitchOffset;

			// set the new value
			m_EffectivePitchOffset = NewValue;

			// figure out if we should notify
			if( m_pParamNode->DoesChangeMustBeNotified( PT_Pitch ) )
			{
				m_pParamNode->PitchNotification( Notified, m_pGameObj );
			}
		}
		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_Volume:
		AKASSERT((in_eTargetType & AkTypeMask) == AkTypeFloat);
		if(in_bIsTerminated)
		{
			m_pvVolumeTransition = NULL;
		}

		// compute the value to be notified
		fNotified = in_unionValue.fValue - m_EffectiveVolumeOffset;

		// set the new value
		m_EffectiveVolumeOffset = in_unionValue.fValue;

		// figure out if we should notify
		if(m_pParamNode->DoesChangeMustBeNotified( PT_Volume ))
		{
			m_pParamNode->VolumeNotification( fNotified, m_pGameObj );
		}
		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_Lfe:
		AKASSERT((in_eTargetType & AkTypeMask) == AkTypeFloat);
		if(in_bIsTerminated)
		{
			m_pvLFETransition = NULL;
		}

		// compute the value to be notified
		fNotified = in_unionValue.fValue - m_EffectiveLFEOffset;

		// set the new value
		m_EffectiveLFEOffset = in_unionValue.fValue;

		// figure out if we should notify
		if(m_pParamNode->DoesChangeMustBeNotified( PT_LFE ))
		{
			m_pParamNode->LFENotification( fNotified, m_pGameObj );
		}
		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_LPF:
		AKASSERT((in_eTargetType & AkTypeMask) == AkTypeFloat);
		if(in_bIsTerminated)
		{
			m_pvLPFTransition = NULL;
		}

		// compute the value to be notified
		fNotified = in_unionValue.fValue - m_EffectiveLPFOffset;

		// set the new value
		m_EffectiveLPFOffset = in_unionValue.fValue;

		// figure out if we should notify
		if(m_pParamNode->DoesChangeMustBeNotified( PT_LPF ))
		{
			m_pParamNode->LPFNotification( fNotified, m_pGameObj );
		}
		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_Mute:

		// set the new value
		m_cMuteLevel = static_cast<AkUInt8>(((in_unionValue.fValue - AK_MINIMUM_VOLUME_LEVEL) * 255.0f) / -(AK_MINIMUM_VOLUME_LEVEL));

		if(in_bIsTerminated)
		{
			m_pvMuteTransition = NULL;
			
			if(m_cMuteLevel == UNMUTED_LVL)
			{
				MONITOR_PARAMCHANGED(
					AkMonitorData::NotificationReason_Unmuted, 
					m_pParamNode->ID(), 
					m_pGameObj?m_pGameObj->ID():AK_INVALID_GAME_OBJECT
					);
			}
		}

		AkMutedMapItem item;
        item.m_bIsPersistent = false;
		item.m_bIsGlobal = (m_pGameObj == NULL);
		item.m_Identifier = m_pParamNode;

		// object wise ?
		if( m_pGameObj != NULL )
		{
			m_pParamNode->MuteNotification(m_cMuteLevel, m_pGameObj, item);
		}
		else
		{
			m_pParamNode->MuteNotification(m_cMuteLevel,item);
		}

		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	case TransTarget_FeedbackVolume:
		AKASSERT((in_eTargetType & AkTypeMask) == AkTypeFloat);
		if(in_bIsTerminated)
		{
			m_pvFeedbackVolumeTransition = NULL;
		}

		// compute the value to be notified
		fNotified = in_unionValue.fValue - m_EffectiveFeedbackVolumeOffset;

		// set the new value
		m_EffectiveFeedbackVolumeOffset = in_unionValue.fValue;

		m_pParamNode->FeedbackVolumeNotification( fNotified, m_pGameObj );
		break;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

	default:
		AKASSERT(!"Unsupported data type");
		break;
	}
}
