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
#include "IBusProxy.h"

class BusProxyLocal : public ParameterableProxyLocal
							, virtual public IBusProxy
{
public:
	BusProxyLocal();
	virtual ~BusProxyLocal();

	virtual void Init( AkUniqueID in_id );

	virtual void Volume( AkReal32 in_volume );
	virtual void LFEVolume( AkReal32 in_LFEVolume );
	virtual void Pitch( AkPitchValue in_Pitch );
	virtual void LPF( AkLPFType in_LPF );

	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride );
	virtual void SetMaxNumInstances( AkUInt16 in_ulMaxNumInstance );
	virtual void SetMaxReachedBehavior( bool in_bKillNewest );

	virtual void AddChild( AkUniqueID in_id );
	virtual void RemoveChild( AkUniqueID in_id );
	virtual void RemoveAllChildren();

	virtual void SetRecoveryTime(AkTimeMs in_recoveryTime);

	virtual void AddDuck(
		AkUniqueID in_busID,
		AkVolumeValue in_duckVolume,
		AkTimeMs in_fadeOutTime,
		AkTimeMs in_fadeInTime,
		AkCurveInterpolation in_eFadeCurve
		);

	virtual void RemoveDuck(
		AkUniqueID in_busID
		);

	virtual void RemoveAllDuck();

	virtual void SetAsBackgroundMusic();
	virtual void UnsetAsBackgroundMusic();

	virtual void EnableWiiCompressor( bool in_bEnable );

	virtual void FeedbackVolume( AkReal32 in_fFeedbackVolume );
	virtual void FeedbackLPF( AkLPFType in_fFeedbackLPF);
};

