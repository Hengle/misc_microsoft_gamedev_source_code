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
// Converter.h
// Implementation of the safety limiter and converter for the DBOX platform.
// This signal processor will ensure that the incoming signal doesn't
// cause speeds and accelerations higher than the designed limits of
// the device.  It will convert and AkInt32erleave the samples.  Finally,
// it will upsample to the correct output rate.
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkCommon.h"
#include "..\Common\DBoxDefs.h"

class CLimiter
{
public:
	CLimiter();
	virtual ~CLimiter();
	void Init(AkUInt32 in_iSampleRate);

	void Limit(AkReal32 *in_pSrc, AkInt32 in_nSamples, const __m128& in_mOutput1, const __m128& in_mOutput2);

protected:

	AkReal32 LimitStep(AkReal32 &X, const AkReal32& X1, const AkReal32 &Y1, const AkReal32 &Y2, AkReal32& io_fError);
	void SetLimits(AkReal32 in_fAccel, AkReal32 in_fSpeed);

	__m128	m_Input1;			//Input -1 (last input)
	__m128	m_AccumulatedError;	//Integrated error between input and output

	AkReal32			m_fTimeSample;		
	AkReal32			m_fSampleRate;
	AkReal32			m_fSampleSquaredHalved;
	AkReal32			m_fSampleRateSquared;
	AkReal32			m_fMaxAcceleration;
	AkReal32			m_fMaxSpeed;
};