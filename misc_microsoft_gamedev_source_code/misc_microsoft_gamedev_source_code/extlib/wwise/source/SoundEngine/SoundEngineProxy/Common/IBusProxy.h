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

class IBusProxy : virtual public IParameterableProxy
{
	DECLARE_BASECLASS( IParameterableProxy );
public:
	virtual void Volume( AkReal32 in_volume ) = 0;
	virtual void LFEVolume( AkReal32 in_LFEVolume ) = 0;
	virtual void Pitch( AkPitchValue in_Pitch ) = 0;
	virtual void LPF( AkLPFType in_LPF ) = 0;

	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride ) = 0;
	virtual void SetMaxNumInstances( AkUInt16 in_ulMaxNumInstance ) = 0;
	virtual void SetMaxReachedBehavior( bool in_bKillNewest ) = 0;

	virtual void AddChild( AkUniqueID in_id ) = 0;
	virtual void RemoveChild( AkUniqueID in_id ) = 0;
	virtual void RemoveAllChildren() = 0;

	virtual void SetRecoveryTime(AkTimeMs in_recoveryTime) = 0;

	virtual void AddDuck(
		AkUniqueID in_busID,
		AkVolumeValue in_duckVolume,
		AkTimeMs in_fadeOutTime,
		AkTimeMs in_fadeInTime,
		AkCurveInterpolation in_eFadeCurve
		) = 0;

	virtual void RemoveDuck(
		AkUniqueID in_busID
		) = 0;

	virtual void RemoveAllDuck() = 0;

	virtual void SetAsBackgroundMusic() = 0;
	virtual void UnsetAsBackgroundMusic() = 0;

	virtual void EnableWiiCompressor( bool in_bEnable ) = 0;

	virtual void FeedbackVolume(AkReal32 in_volume) = 0;

	virtual void FeedbackLPF(AkLPFType in_feedbackLPF) = 0;

	enum MethodIDs
	{
		MethodVolume = __base::LastMethodID,
		MethodLFEVolume,
		MethodPitch,
		MethodLPF,

		MethodSetMaxNumInstancesOverrideParent,
		MethodSetMaxNumInstances,
		MethodSetMaxReachedBehavior,

		MethodAddChild,
		MethodRemoveChild,
		MethodRemoveAllChildren,

		MethodSetRecoveryTime,
		MethodAddDuck,
		MethodRemoveDuck,
		MethodRemoveAllDuck,

		MethodSetAsBackgroundMusic,
		MethodUnsetAsBackgroundMusic,

		MethodEnableWiiCompressor,

		MethodFeedbackVolume,
		MethodFeedbackLPF,
		
		LastMethodID
	};

};

