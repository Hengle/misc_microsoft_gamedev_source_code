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
// Ak3DParams.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _3D_PARAMETERS_H_
#define _3D_PARAMETERS_H_

#include "AkLEngineDefs.h"		// AkPositioningType

struct ConeParams
{
	AkReal32		fInsideAngle;					// radians
	AkReal32		fOutsideAngle;					// radians
	AkReal32		fOutsideVolume;
	AkLPFType		LoPass;
};

struct AkSoundParams
{
	AkVolumeValue		Volume;
	AkVolumeValue		LFE;
	AkPitchValue		Pitch;
	AkLPFType			LPF;
};

struct AkSoundPositioningParams
{
	AkPositioningType	ePosType;
	AkSoundPosition		Position;
	ConeParams			Cone;
	AkReal32			fDivergenceCenter; // 0..1
	AkReal32			fPAN_RL;
	AkReal32			fPAN_FR;
	AkUInt32			uListenerMask;
};

#endif //_3D_PARAMETERS_H_
