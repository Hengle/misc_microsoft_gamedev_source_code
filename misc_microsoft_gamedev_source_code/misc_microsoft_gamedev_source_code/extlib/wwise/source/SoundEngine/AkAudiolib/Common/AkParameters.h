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
// AkParameters.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

#include "AkPrivateTypes.h"

template <class T_Type>
struct RANGED_MODIFIERS
{
	T_Type m_min;
	T_Type m_max;
	RANGED_MODIFIERS()
		:m_min(0)
		,m_max(0)
	{}
};

template <class T_Type>
struct RANGED_PARAMETER
{
	T_Type m_base;
	RANGED_MODIFIERS<T_Type> m_mod;
	RANGED_PARAMETER()
		:m_base(0)
	{}
};

struct AkLoop
{
	AkInt16 lLoopCount;		// Number of loop before continue
	AkUInt8 bIsEnabled  :1;	// Is Looping enabled
	AkUInt8 bIsInfinite :1;	// Is looping infinite
	AkLoop()
		:lLoopCount(0)
		,bIsEnabled(false)
		,bIsInfinite(false)
	{}
};

enum AkValueMeaning
{
	AkValueMeaning_Default		= 0,		//Use default parameter instead
	AkValueMeaning_Independent	= 1,		//Use this parameter directly (also means override when on an object)
	AkValueMeaning_Offset		= 2			//Use this parameter as an offset from the default value
#define VALUE_MEANING_NUM_STORAGE_BIT 4
};

// Also called "Les sept parametres"
struct AkStdParameters
{
	AkVolumeValue	Volume;						// Volume
	AkVolumeValue	LFEVolume;					// Low frequency effect Volume
	AkPitchValue	Pitch;						// Pitch
	AkLPFType		LPF;						// LPF
	AkValueMeaning	eVolumeValueMeaning :VALUE_MEANING_NUM_STORAGE_BIT;		// Volume Value meaning
	AkValueMeaning	eLFEValueMeaning 	:VALUE_MEANING_NUM_STORAGE_BIT;		// LFE Value meaning
	AkValueMeaning	ePitchValueMeaning 	:VALUE_MEANING_NUM_STORAGE_BIT;		// Pitch Value meaning
	AkValueMeaning	eLPFValueMeaning 	:VALUE_MEANING_NUM_STORAGE_BIT;		// LPF Value meaning
	AkStdParameters()
	{
		eVolumeValueMeaning = AkValueMeaning_Default;
		eLFEValueMeaning = AkValueMeaning_Default;
		ePitchValueMeaning = AkValueMeaning_Default;
		eLPFValueMeaning = AkValueMeaning_Default;
		Volume = 0;
		LFEVolume = 0;
		Pitch = 0;
		LPF = 0;
	}
};

struct AkPBIModValues
{
	AkVolumeValue	VolumeOffset;
	AkVolumeValue	LFEOffset;
	AkPitchValue	PitchOffset;
	AkLPFType		LPFOffset;
	AkPBIModValues()
		:VolumeOffset(0)
		,LFEOffset(0)
		,PitchOffset(0)
		,LPFOffset(0)
	{}
};
#endif
