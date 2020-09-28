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

#include "IParameterableProxy.h"
#include "AkSharedEnum.h"
#include "AKParameters.h"

struct AkPathVertex;
struct AkPathListItemOffset;

class IParameterNodeProxy : virtual public IParameterableProxy
{
	DECLARE_BASECLASS( IParameterableProxy );
public:
	virtual void Volume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax ) = 0;
	virtual void Pitch( AkPitchValue in_pitch, AkPitchValue in_rangeMin, AkPitchValue in_rangeMax ) = 0;
	virtual void LPF( AkLPFType in_LPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax ) = 0;
	virtual void LFEVolume( AkReal32 in_LFEVolume, AkReal32 in_rangeMin, AkReal32 in_rangeMax ) = 0;

	virtual void PosSetPositioningType( AkPositioningType in_ePosType ) = 0;
	virtual void PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled ) = 0;
	virtual void PosSetAttenuationID( AkUniqueID in_AttenuationID ) = 0;

	virtual void PosSetCenterPct( AkInt in_iCenterPct ) = 0;

	virtual void PosSetPAN_RL( AkReal32 in_fPanRL ) = 0;
	virtual void PosSetPAN_FR( AkReal32 in_fPanFR ) = 0;
	virtual void PosSetPannerEnabled( bool in_bIsPannerEnabled ) = 0;

	virtual void PosSetIsPositionDynamic( bool in_bIsDynamic ) = 0;

	virtual void PosSetPathMode( AkPathMode in_ePathMode ) = 0;
	virtual void PosSetIsLooping( bool in_bIsLooping ) = 0;
	virtual void PosSetTransition( AkTimeMs in_TransitionTime ) = 0;

	virtual void PosSetPath(
		AkPathVertex*           in_pArayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		) = 0;

	virtual void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		const AkVector& in_ptPosition,
		AkTimeMs in_DelayToNext
		) = 0;

	virtual void OverrideFXParent( bool in_bIsFXOverrideParent ) = 0;

	virtual void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior ) = 0;
	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride ) = 0;
	virtual void SetVVoicesOptOverrideParent( bool in_bOverride ) = 0;
	virtual void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance ) = 0;
	virtual void SetMaxReachedBehavior( bool in_bKillNewest ) = 0;
	virtual void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior ) = 0;

	virtual void FeedbackVolume( AkReal32 in_volume, AkReal32 in_rangeMin, AkReal32 in_rangeMax ) = 0;

	virtual void FeedbackLPF( AkLPFType in_FeedbackLPF, AkLPFType in_rangeMin, AkLPFType in_rangeMax ) = 0;

	enum MethodIDs
	{
		MethodVolume = __base::LastMethodID,
		MethodPitch,
		MethodLFEVolume,
		MethodLPF,

		MethodPosSetPositioningType,
		MethodPosSetSpatializationEnabled,
		MethodPosSetAttenuationID,

		MethodPosSetCenterPct,

		MethodPosSetPAN_RL,
		MethodPosSetPAN_FR,
		MethodPosPosSetPannerEnabled,

		MethodPosSetIsPositionDynamic,

		MethodPosSetPathMode,
		MethodPosSetIsLooping,
		MethodPosSetTransition,

		MethodPosSetPath,

		MethodPosUpdatePathPoint,

		MethodOverrideFXParent,

		MethodSetBelowThresholdBehavior,
		MethodSetSetMaxNumInstancesOverrideParent,
		MethodSetVVoicesOptOverrideParent,
		MethodSetMaxNumInstances,
		MethodSetMaxReachedBehavior,
		MethodSetVirtualQueueBehavior,

		MethodFeedbackVolume,
		MethodFeedbackLPF,

		LastMethodID
	};
};
