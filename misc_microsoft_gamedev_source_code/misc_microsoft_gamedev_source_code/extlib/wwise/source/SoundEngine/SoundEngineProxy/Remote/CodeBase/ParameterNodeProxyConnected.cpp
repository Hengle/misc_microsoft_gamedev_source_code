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

#include "ParameterNodeProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


ParameterNodeProxyConnected::ParameterNodeProxyConnected()
{
}

ParameterNodeProxyConnected::~ParameterNodeProxyConnected()
{
}

void ParameterNodeProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ParameterNodeProxyLocal& rLocalProxy = static_cast<ParameterNodeProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IParameterNodeProxy::MethodVolume:
		{
			ParameterNodeProxyCommandData::Volume volume;
			in_rSerializer.Get( volume );

			rLocalProxy.Volume( volume.m_volume, volume.m_rangeMin, volume.m_rangeMax );
			break;
		}

	case IParameterNodeProxy::MethodPitch:
		{
			ParameterNodeProxyCommandData::Pitch pitch;
			in_rSerializer.Get( pitch );

			rLocalProxy.Pitch( pitch.m_pitchType, pitch.m_rangeMin, pitch.m_rangeMax );
			break;
		}

	case IParameterNodeProxy::MethodLFEVolume:
		{
			ParameterNodeProxyCommandData::LFEVolume lfeVolume;
			in_rSerializer.Get( lfeVolume );

			rLocalProxy.LFEVolume( lfeVolume.m_LFEVolume, lfeVolume.m_rangeMin, lfeVolume.m_rangeMax );
			break;
		}

	case IParameterNodeProxy::MethodLPF:
		{
			ParameterNodeProxyCommandData::LPF lpf;
			in_rSerializer.Get( lpf );

			rLocalProxy.LPF( lpf.m_LPF, lpf.m_rangeMin, lpf.m_rangeMax );
			break;
		}

	case IParameterNodeProxy::MethodPosSetSpatializationEnabled:
		{
			ParameterNodeProxyCommandData::PosSetSpatializationEnabled SetSpatializationEnabledParams;
			in_rSerializer.Get( SetSpatializationEnabledParams );

			rLocalProxy.PosSetSpatializationEnabled( SetSpatializationEnabledParams.m_bIsSpatializationEnabled );

			break;
		}

	case IParameterNodeProxy::MethodPosSetAttenuationID:
		{
			ParameterNodeProxyCommandData::PosSetAttenuationID SetAttenuationIDParams;
			in_rSerializer.Get( SetAttenuationIDParams );

			rLocalProxy.PosSetAttenuationID( SetAttenuationIDParams.m_uAttenuationID );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPositioningType:
		{
			ParameterNodeProxyCommandData::PosSetPositioningType posSetPositioningType;
			in_rSerializer.Get( posSetPositioningType );
			
			rLocalProxy.PosSetPositioningType( posSetPositioningType.m_ePosType );

			break;
		}

	case IParameterNodeProxy::MethodPosSetCenterPct:
		{
			ParameterNodeProxyCommandData::PosSetCenterPct posSetCenterPct;
			in_rSerializer.Get( posSetCenterPct );
			
			rLocalProxy.PosSetCenterPct( posSetCenterPct.m_iCenterPct );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPAN_RL:
		{
			ParameterNodeProxyCommandData::PosSetPAN_RL posSetPAN_RL;
			in_rSerializer.Get( posSetPAN_RL );
			
			rLocalProxy.PosSetPAN_RL( posSetPAN_RL.m_fPanRL );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPAN_FR:
		{
			ParameterNodeProxyCommandData::PosSetPAN_FR posSetPAN_FR;
			in_rSerializer.Get( posSetPAN_FR );
			
			rLocalProxy.PosSetPAN_FR( posSetPAN_FR.m_fPanFR );

			break;
		}

	case IParameterNodeProxy::MethodPosPosSetPannerEnabled:
		{
			ParameterNodeProxyCommandData::PosSetPannerEnabled posSetPannerEnabled;
			in_rSerializer.Get( posSetPannerEnabled );
			
			rLocalProxy.PosSetPannerEnabled( posSetPannerEnabled.m_bIsPannerEnabled );

			break;
		}

	case IParameterNodeProxy::MethodPosSetIsPositionDynamic:
		{
			ParameterNodeProxyCommandData::PosSetIsPositionDynamic posSetIsPositionDynamic;
			in_rSerializer.Get( posSetIsPositionDynamic );
			
			rLocalProxy.PosSetIsPositionDynamic( posSetIsPositionDynamic.m_bIsDynamic );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPathMode:
		{
			ParameterNodeProxyCommandData::PosSetPathMode posSetPathMode;
			in_rSerializer.Get( posSetPathMode );
			
			rLocalProxy.PosSetPathMode( posSetPathMode.m_ePathMode );

			break;
		}

	case IParameterNodeProxy::MethodPosSetIsLooping:
		{
			ParameterNodeProxyCommandData::PosSetIsLooping posSetIsLooping;
			in_rSerializer.Get( posSetIsLooping );
			
			rLocalProxy.PosSetIsLooping( posSetIsLooping.m_bIsLooping );

			break;
		}

	case IParameterNodeProxy::MethodPosSetTransition:
		{
			ParameterNodeProxyCommandData::PosSetTransition posSetTransition;
			in_rSerializer.Get( posSetTransition );
			
			rLocalProxy.PosSetTransition( posSetTransition.m_transitionTime );

			break;
		}

	case IParameterNodeProxy::MethodPosSetPath:
		{
			ParameterNodeProxyCommandData::PosSetPath posSetPath;
			in_rSerializer.Get( posSetPath );
			
			rLocalProxy.PosSetPath( posSetPath.m_pArrayVertex, posSetPath.m_ulNumVertices, posSetPath.m_pArrayPlaylist, posSetPath.m_ulNumPlaylistItem );

			break;
		}

	case IParameterNodeProxy::MethodPosUpdatePathPoint:
		{
			ParameterNodeProxyCommandData::PosUpdatePathPoint posUpdatePathPoint;
			in_rSerializer.Get( posUpdatePathPoint );
			
			rLocalProxy.PosUpdatePathPoint( posUpdatePathPoint.m_ulPathIndex, posUpdatePathPoint.m_ulVertexIndex, posUpdatePathPoint.m_ptPosition, posUpdatePathPoint.m_delayToNext );

			break;
		}

	case IParameterNodeProxy::MethodOverrideFXParent:
		{
			ParameterNodeProxyCommandData::OverrideFXParent overrideFXParent;
			in_rSerializer.Get( overrideFXParent );
			
			rLocalProxy.OverrideFXParent( overrideFXParent.m_bIsFXOverrideParent );

			break;
		}
	case IParameterNodeProxy::MethodSetBelowThresholdBehavior:
		{
			ParameterNodeProxyCommandData::SetBelowThresholdBehavior setBelowThresholdBehavior;
			in_rSerializer.Get( setBelowThresholdBehavior );
			
			rLocalProxy.SetBelowThresholdBehavior( setBelowThresholdBehavior.m_eBelowThresholdBehavior );

			break;
		}
	case IParameterNodeProxy::MethodSetSetMaxNumInstancesOverrideParent:
		{
			ParameterNodeProxyCommandData::SetMaxNumInstancesOverrideParent setMaxNumInstancesOverrideParent;
			in_rSerializer.Get( setMaxNumInstancesOverrideParent );
			
			rLocalProxy.SetMaxNumInstancesOverrideParent( setMaxNumInstancesOverrideParent.m_bOverride );

			break;
		}
	case IParameterNodeProxy::MethodSetVVoicesOptOverrideParent:
		{
			ParameterNodeProxyCommandData::SetVVoicesOptOverrideParent setVVoicesOptOverrideParent;
			in_rSerializer.Get( setVVoicesOptOverrideParent );
			
			rLocalProxy.SetVVoicesOptOverrideParent( setVVoicesOptOverrideParent.m_bOverride );

			break;
		}
	case IParameterNodeProxy::MethodSetMaxNumInstances:
		{
			ParameterNodeProxyCommandData::SetMaxNumInstances setMaxNumInstances;
			in_rSerializer.Get( setMaxNumInstances );
			
			rLocalProxy.SetMaxNumInstances( setMaxNumInstances.m_u16MaxNumInstance );

			break;
		}
	case IParameterNodeProxy::MethodSetMaxReachedBehavior:
		{
			ParameterNodeProxyCommandData::SetMaxReachedBehavior setMaxReachedBehavior;
			in_rSerializer.Get( setMaxReachedBehavior );
			
			rLocalProxy.SetMaxReachedBehavior( setMaxReachedBehavior.m_bKillNewest );

			break;
		}
	case IParameterNodeProxy::MethodSetVirtualQueueBehavior:
		{
			ParameterNodeProxyCommandData::SetVirtualQueueBehavior setVirtualQueueBehavior;
			in_rSerializer.Get( setVirtualQueueBehavior );
			
			rLocalProxy.SetVirtualQueueBehavior( setVirtualQueueBehavior.m_eBehavior );

			break;
		}
	case IParameterNodeProxy::MethodFeedbackVolume:
		{
			ParameterNodeProxyCommandData::FeedbackVolume oFeedbackVolume;
			in_rSerializer.Get( oFeedbackVolume );

			rLocalProxy.FeedbackVolume( oFeedbackVolume.m_FeedbackVolume, oFeedbackVolume.m_rangeMin, oFeedbackVolume.m_rangeMax );
			break;
		}
	case IParameterNodeProxy::MethodFeedbackLPF :
		{
			ParameterNodeProxyCommandData::FeedbackLPF oFeedbackLPF;
			in_rSerializer.Get( oFeedbackLPF );

			rLocalProxy.FeedbackLPF( oFeedbackLPF.m_FeedbackLPF, oFeedbackLPF.m_rangeMin, oFeedbackLPF.m_rangeMax );
			break;
		}
	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
