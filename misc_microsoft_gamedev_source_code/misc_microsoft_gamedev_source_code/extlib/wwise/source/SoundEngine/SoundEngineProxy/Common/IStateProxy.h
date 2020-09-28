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

#include "IObjectProxy.h"

#include "AkParameters.h"

class IStateProxy : virtual public IObjectProxy
{
	DECLARE_BASECLASS( IObjectProxy );
public:
    virtual void Volume( AkReal32 in_volume ) = 0;
    virtual void VolumeMeaning( AkValueMeaning in_eMeaning ) = 0;
    virtual void Pitch( AkPitchValue in_pitch ) = 0;
    virtual void PitchMeaning( AkValueMeaning in_eMeaning ) = 0;
	virtual void LPF( AkLPFType in_LPF ) = 0;
    virtual void LPFMeaning( AkValueMeaning in_eMeaning ) = 0;
    virtual void LFEVolume( AkReal32 in_LFEVolume ) = 0;
    virtual void LFEVolumeMeaning( AkValueMeaning in_eMeaning ) = 0;

	enum MethodIDs
	{
		MethodVolume = __base::LastMethodID,
		MethodVolumeMeaning,
		MethodPitch,
		MethodPitchMeaning,
		MethodLPF,
		MethodLPFMeaning,
		MethodLFEVolume,
		MethodLFEVolumeMeaning,

		LastMethodID
	};
};

