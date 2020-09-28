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

///////////////////////////////////////////////////////////////////////
//
// PRHMaths.cpp
//
// PitchRollHeave math functions, shared between the Wwise plugin and sound engine plugin.
//
// Copyright 2008 Audiokinetic Inc. / All Rights Reserved
//
///////////////////////////////////////////////////////////////////////

#include "DBoxPRHMaths.h"
#include "math.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

float GetHeaveLimit(float in_fFrontRight, float in_fFrontLeft, float in_fBackLeft, float in_fBackRight)
{
	//Get the maximum possible change and ensure all actuators stay in the limits.
	//Keep the pitch and roll values intact.
	float fMin = c_fUIMax;
	float fMax = -c_fUIMax;
	fMin = AkMin(in_fFrontLeft, in_fFrontRight);
	fMax = AkMax(in_fFrontLeft, in_fFrontRight);

	fMin = AkMin(in_fBackLeft, fMin);
	fMax = AkMax(in_fBackLeft, fMax);

	fMin = AkMin(in_fBackRight, fMin);
	fMax = AkMax(in_fBackRight, fMax);

	//Compute the remaining displacement.
	return (c_fUIMax * 2 - (fMax - fMin)) / 2;
}

float GetAngleLimit(float in_fOtherAngle, float in_fHeave)
{
	float fHeave = UserToHeave(in_fHeave);
	float fAngle = UserToAngle(in_fOtherAngle);
	float fAngleY = c_fSide * sin(fAngle);
	float fCenterLow = fHeave - fAngleY;
	float fCenterHigh =fHeave + fAngleY; 

	float fRemaining = c_fMaxHeave - fabs(AkMax(fCenterLow, fCenterHigh));
	float fMaxAngle = asin(fRemaining / c_fSide);

	//Limit the value
	return fabs(AngleToUser(fMaxAngle));
}

float GetActuatorLimit(float in_fHeave)
{
	return c_fUIMax - fabs(in_fHeave);
}

void ClampHeave(float in_fFrontRight, float in_fFrontLeft, float in_fBackLeft, float in_fBackRight, float &io_fHeave)
{
	float fHeaveMax = GetHeaveLimit(in_fFrontRight, in_fFrontLeft, in_fBackLeft, in_fBackRight);
	if (io_fHeave > fHeaveMax)
		io_fHeave = fHeaveMax;
	if (io_fHeave < -fHeaveMax)
		io_fHeave = -fHeaveMax;
}

void ClampAngle(float in_fOtherAngle, float in_fHeave, float &io_fAngle)
{
	float fMaxAngle = GetAngleLimit(in_fOtherAngle, in_fHeave);
	if (io_fAngle > fMaxAngle)
		io_fAngle = fMaxAngle;
	if (io_fAngle < -fMaxAngle)
		io_fAngle = -fMaxAngle;
}

void ClampActuator(float in_fHeave, float &io_fActuator)
{
	float fMaxActuator = GetActuatorLimit(in_fHeave);
	if (io_fActuator > in_fHeave + fMaxActuator)
		io_fActuator = in_fHeave + fMaxActuator;
	if (io_fActuator < in_fHeave - fMaxActuator)
		io_fActuator = in_fHeave - fMaxActuator;
}

void ComputeActuatorsFromPRH(float in_fPitch, float in_fRoll, float in_fHeave,
							 float &out_fFR, float &out_fFL, float &out_fBL, float &out_fBR)
{
	//Add the pitch component
	float fPitchComponent = c_fSide * sin(UserToAngle(in_fPitch));
	out_fFL = -fPitchComponent;
	out_fFR = -fPitchComponent;
	out_fBR = fPitchComponent;
	out_fBL = fPitchComponent;

	//Add the roll component
	float fRollComponent = c_fSide * sin(UserToAngle(in_fRoll));
	out_fFR += fRollComponent ;
	out_fBR += fRollComponent;
	out_fBL += -fRollComponent;
	out_fFL += -fRollComponent;

	//Change the values back in user units.
	out_fFL = HeaveToUser(out_fFL) + in_fHeave;
	out_fFR = HeaveToUser(out_fFR) + in_fHeave;
	out_fBL = HeaveToUser(out_fBL) + in_fHeave;
	out_fBR = HeaveToUser(out_fBR) + in_fHeave;
}
