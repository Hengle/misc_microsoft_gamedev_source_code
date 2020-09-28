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


#pragma once

#include "ParameterableProxyLocal.h"
#include "IParameterNodeProxy.h"

class CAkParameterNode;

class ParameterNodeProxyLocal : public ParameterableProxyLocal
								, virtual public IParameterNodeProxy
{
public:
	// IParameterableProxy members
	virtual void Volume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
    virtual void Pitch( AkPitchValue in_pitch, AkPitchValue in_rangeMin, AkPitchValue in_rangeMax );
	virtual void LPF( AkLPFType in_LPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax );
    virtual void LFEVolume( AkReal32 in_LFEVolume, AkReal32 in_rangeMin, AkReal32 in_rangeMax );

	virtual void PosSetPositioningType( AkPositioningType in_ePosType );
	virtual void PosSetSpatializationEnabled( bool in_bSpatializationEnabled );
	virtual void PosSetAttenuationID( AkUniqueID in_AttenuationID );

	virtual void PosSetCenterPct( AkInt in_iCenterPct );

	virtual void PosSetPAN_RL( AkReal32 in_fPanRL );
	virtual void PosSetPAN_FR( AkReal32 in_fPanFR );
	virtual void PosSetPannerEnabled( bool in_bIsPannerEnabled );

	virtual void PosSetIsPositionDynamic( bool in_bIsDynamic );

	virtual void PosSetPathMode( AkPathMode in_ePathMode );
	virtual void PosSetIsLooping( bool in_bIsLooping );
	virtual void PosSetTransition( AkTimeMs in_TransitionTime );

	virtual void PosSetPath(
		AkPathVertex*           in_pArayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

	virtual void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		const AkVector& in_ptPosition,
		AkTimeMs in_DelayToNext
		);

	virtual void OverrideFXParent( bool in_bIsFXOverrideParent );

	virtual void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior );
	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride );
	virtual void SetVVoicesOptOverrideParent( bool in_bOverride );
	virtual void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance );
	virtual void SetMaxReachedBehavior( bool in_bKillNewest );
	virtual void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior );

	virtual void FeedbackVolume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
	virtual void FeedbackLPF( AkLPFType in_FeedbackLPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax );
};
