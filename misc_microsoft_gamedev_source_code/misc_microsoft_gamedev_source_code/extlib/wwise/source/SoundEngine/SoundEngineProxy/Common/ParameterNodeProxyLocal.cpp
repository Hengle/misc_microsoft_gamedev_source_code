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

#include "ParameterNodeProxyLocal.h"
#include "AkParameterNode.h"
#include "AkCritical.h"


void ParameterNodeProxyLocal::Volume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Volume( in_volume, in_rangeMin, in_rangeMax );
	}
}

void ParameterNodeProxyLocal::Pitch( AkPitchValue in_pitch, AkPitchValue in_rangeMin, AkPitchValue in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Pitch( in_pitch, in_rangeMin, in_rangeMax );
	}
}

void ParameterNodeProxyLocal::LPF( AkLPFType in_LPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LPF( in_LPF, in_rangeMin, in_rangeMax );
	}
}
 
void ParameterNodeProxyLocal::LFEVolume( AkReal32 in_LFEVolume, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LFEVolume( in_LFEVolume, in_rangeMin, in_rangeMax );
	}
}

void ParameterNodeProxyLocal::PosSetPositioningType( AkPositioningType in_ePosType )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPositioningType( in_ePosType );
	}
}

void ParameterNodeProxyLocal::PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetSpatializationEnabled( in_bIsSpatializationEnabled );
	}
}

void ParameterNodeProxyLocal::PosSetAttenuationID( AkUniqueID in_AttenuationID )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetAttenuationID( in_AttenuationID );
	}
}


void ParameterNodeProxyLocal::PosSetCenterPct( AkInt in_iCenterPct )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetCenterPct( in_iCenterPct );
	}
}

void ParameterNodeProxyLocal::PosSetPAN_RL( AkReal32 in_fPanRL )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPAN_RL( in_fPanRL );
	}
}

void ParameterNodeProxyLocal::PosSetPAN_FR( AkReal32 in_fPanFR )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPAN_FR( in_fPanFR );
	}
}

void ParameterNodeProxyLocal::PosSetPannerEnabled( bool in_bIsPannerEnabled )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPannerEnabled( in_bIsPannerEnabled );
	}
}

void ParameterNodeProxyLocal::PosSetIsPositionDynamic( bool in_bIsDynamic )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetIsPositionDynamic( in_bIsDynamic );
	}
}

void ParameterNodeProxyLocal::PosSetPathMode( AkPathMode in_ePathMode )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPathMode( in_ePathMode );
	}
}

void ParameterNodeProxyLocal::PosSetIsLooping( bool in_bIsLooping )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetIsLooping( in_bIsLooping );
	}
}

void ParameterNodeProxyLocal::PosSetTransition( AkTimeMs in_TransitionTime )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetTransition( in_TransitionTime );
	}
}

void ParameterNodeProxyLocal::PosSetPath(
	AkPathVertex*           in_pArayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->PosSetPath( in_pArayVertex, in_ulNumVertices, in_pArrayPlaylist, in_ulNumPlaylistItem );
	}
}

void ParameterNodeProxyLocal::PosUpdatePathPoint(
	AkUInt32 in_ulPathIndex,
	AkUInt32 in_ulVertexIndex,
	const AkVector& in_ptPosition,
	AkTimeMs in_DelayToNext
	)
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // WG-4418 eliminate potential problems

		pIndexable->PosUpdatePathPoint( in_ulPathIndex, in_ulVertexIndex, in_ptPosition, in_DelayToNext );
	}
}

void ParameterNodeProxyLocal::OverrideFXParent( bool in_bIsFXOverrideParent )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->OverrideFXParent( in_bIsFXOverrideParent );
	}
}

void ParameterNodeProxyLocal::SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetBelowThresholdBehavior( in_eBelowThresholdBehavior );
	}
}

void ParameterNodeProxyLocal::SetMaxNumInstancesOverrideParent( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxNumInstOverrideParent( in_bOverride ); 
	}
}

void ParameterNodeProxyLocal::SetVVoicesOptOverrideParent( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetVVoicesOptOverrideParent( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxNumInstances( in_u16MaxNumInstance );
	}
}

void ParameterNodeProxyLocal::SetMaxReachedBehavior( bool in_bKillNewest )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxReachedBehavior( in_bKillNewest );
	}
}

void ParameterNodeProxyLocal::SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetVirtualQueueBehavior( in_eBehavior );
	}
}

void ParameterNodeProxyLocal::FeedbackVolume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->FeedbackVolume( in_volume, in_rangeMin, in_rangeMax );
	}
}

void ParameterNodeProxyLocal::FeedbackLPF( AkLPFType in_FeedbackLPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->FeedbackLPF( in_FeedbackLPF, in_rangeMin, in_rangeMax );
	}
}
