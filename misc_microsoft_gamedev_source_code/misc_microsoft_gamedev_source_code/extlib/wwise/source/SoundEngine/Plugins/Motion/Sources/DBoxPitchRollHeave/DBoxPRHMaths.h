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

#pragma once

const float c_fSide = 27.0f * 0.0254f / 2;	//27 inches, in mm.
const float c_fMaxHeave = 0.035f / 2;	//3.5 cm of displacement
const float c_fNormalizeHeave = 1.0f / c_fMaxHeave;
const float c_fMaxAngle = 0.0510574677f; //arcsin(0.035 mm / 27 inches);	Maximum angle, in rads.
const float c_fUIMax = 100;

inline float AngleToUser(float in_fAngle)
{
	return -c_fUIMax * in_fAngle / c_fMaxAngle;
}
inline float UserToAngle(float in_fUser)
{
	return c_fMaxAngle * in_fUser / -c_fUIMax;
}

inline float HeaveToUser(float in_fHeave)
{
	return c_fUIMax * in_fHeave / c_fMaxHeave;
}
inline float UserToHeave(float in_fUser)
{
	return c_fMaxHeave * in_fUser / c_fUIMax;
}

float GetHeaveLimit(float in_fFrontRight, float in_fFrontLeft, float in_fBackLeft, float in_fBackRight);
float GetAngleLimit(float in_fOtherAngle, float in_fHeave);
float GetActuatorLimit(float in_fHeave);

void ClampHeave(float in_fFrontRight, float in_fFrontLeft, float in_fBackLeft, float in_fBackRight, float &io_fHeave);
void ClampAngle(float in_fOtherAngle, float in_fHeave, float &io_fAngle);
void ClampActuator(float in_fHeave, float &io_fActuator);

void ComputeActuatorsFromPRH(float in_fPitch, float in_fRoll, float in_fHeave,
							 float &out_fFL, float &out_fFR, float &out_fBL, float &out_fBR);
