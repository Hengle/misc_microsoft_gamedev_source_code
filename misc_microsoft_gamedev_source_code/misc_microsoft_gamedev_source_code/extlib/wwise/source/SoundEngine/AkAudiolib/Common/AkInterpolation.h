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
// AkInterpolation.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _INTERPOLATION_H_
#define _INTERPOLATION_H_

#include "AkInterpolationTable.h"
#include "AkPrivateTypes.h"

enum AkITable
{
	AkITable_None = 0x0,
	AkITable_Exp1 = 0x1,
	AkITable_Exp2 = 0x2,
	AkITable_Exp3 = 0x4,
	AkITable_All  = 0xFF
};

class CAkInterpolation
{
public:
	CAkInterpolation();
	~CAkInterpolation();

	AKRESULT Init(size_t in_wTableSize, size_t in_wdBTableSize, AkITable in_eITable = AkITable_All);
	void Term();

	// interpolat and output dBs. Note that AkCurveInterpolation_Constant is not supported.
	AkReal32 InterpolateDB(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve);

	// interpolate and output reals. Note that AkCurveInterpolation_Constant is not supported.
	AkReal32 Interpolate(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve);
	AkReal32 InterpolateNoCheck(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve);

	// convert to real
	AkForceInline AkReal32 dBToReal(AkReal32 in_fdBs)
	{
		return m_tableDBtoReal->Get(in_fdBs);
	}

	// convert to dB
	AkReal32 RealTodB(AkReal32 in_fReal);

	// Convert a linear-scaled 0-255 muting value into a dB-scaled 0-255
	// muting value. For example, 127 will become 238 and will produce
	// a muting volume of around -6 dB.
	AkForceInline AkReal32 LinearMutingTodBMuting255(AkReal32 in_fLinearMuting)
	{
		// only used by layers. and layers guarantee 0-255 range.
		return m_tableLinearMutingTodBMuting255->GetNoCheck(in_fLinearMuting);
	}

	// Convert a linear-scaled -96.3 to +96.3 volume value into a dB-scaled
	// -96.3 to +96.3 volume value. For example, -48.15 will convert to around -6 dB.
	AkForceInline AkReal32 LinearMutingTodBMuting96(AkReal32 in_fLinearVolume)
	{
		return m_tableLinearMutingTodBMuting96->Get(in_fLinearVolume);
	}

private:
	CAkInterpolationTable*	m_tableDBtoReal;
	CAkInterpolationTable*	m_tableRealToDB;
	CAkInterpolationTable*	m_tableExp1;
	CAkInterpolationTable*	m_tableExp2;
	CAkInterpolationTable*	m_tableExp3;
	CAkInterpolationTable*	m_tableLinearMutingTodBMuting255;
	CAkInterpolationTable*	m_tableLinearMutingTodBMuting96;
};
#endif
