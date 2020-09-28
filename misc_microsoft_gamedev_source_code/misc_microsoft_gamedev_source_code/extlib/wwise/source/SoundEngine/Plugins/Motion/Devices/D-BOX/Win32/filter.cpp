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

#include "stdafx.h"
#include "filter.h"
#include <memory.h>
#include <malloc.h>
#include <math.h>

CIIRFilter::CIIRFilter()
{
	Reset();
}

CIIRFilter::~CIIRFilter()
{
}

void CIIRFilter::Reset()
{
	for(int i = 0; i < 8; i++)
		memX[i] = 0.0;

	for(int i = 0; i < 12; i++)
		memY[i] = 0.0;
}

void CIIRFilter::Execute(float *X, float *Y, unsigned int len)
{
	// The filter was designed with the following parameters:
	// Cut-off frequency: 100 Hz
	// Stop-Band: 200 Hz
	// Stop-Band Attenuation: 96.3 dB
	// Ripple: 1 dB
	// Using LabView to design the filter, this gives an elliptic 7th order (ouch).
	static const float b[8] = {-1.9988392744348762,
								0.99900961053468884,
								-1.9968172236412562,
								0.99693675328119269,
								-1.9949340638238202,
								0.9497989994216218,
								-0.99704640075325601,
								0.0};
	static const float a[12] = {0.032678282897244608,
								-0.065337850337712930,
								0.032678282897244608,
								0.032678282897244608,
								-0.065329168674816945,
								0.032678282897244608,
								0.032678282897244608,
								-0.065274313665263553,
								0.032678282897244608,
								0.032678282897244608,
								0.032678282897244608,
								0.0};

	//TODO, take care of clipping

	//Sum the previous inputs for continuity
	for(int i = 0; i < 8; i++)
	{
		float sum = 0;
		//Sum the current inputs (the ones in the current buffer)
		int coef = 0;
		for(int j = i; j >= 0; j--, coef++)
			sum += b[coef] * X[j];

		//Sum the old inputs (the ones kept from the last pass)
		for(int j = 7; j > i; j--, coef++)
			sum += b[coef] * memX[j];

		coef = 0;
		//Sum the current outputs (the ones in the current buffer)
		for(int j = i-1; j >= 0; j--, coef++)
			sum += a[coef] * Y[j];

		//Sum the old outputs (the ones kept from the last pass)
		for(int j = 11; j > i; j--, coef++)
			sum += a[coef] * memY[j];

		Y[i] = sum;
	}

	//Previous inputs aren't used for the next 4.
	for(int i = 8; i < 12; i++)
	{
		float sum = 0;
		//Sum the current inputs (unrolled)
		sum += b[0] * X[i];
		sum += b[1] * X[i-1];
		sum += b[2] * X[i-2];
		sum += b[3] * X[i-3];
		sum += b[4] * X[i-4];
		sum += b[5] * X[i-5];
		sum += b[6] * X[i-6];
		sum += b[7] * X[i-7];

		int coef = 0;
		//Sum the current outputs (the ones in the current buffer)
		for(int j = i-1; j >= 0; j--, coef++)
			sum += a[coef] * Y[j];

		//Sum the old outputs (the ones kept from the last pass)
		for(int j = 11; j > i; j--, coef++)
			sum += a[coef] * memY[j];

		Y[i] = sum;
	}

	for(int i = 12; i < len; i++)
	{
		//Sum the inputs (unrolled)
		float sum = 0;
		sum += b[0] * X[i];
		sum += b[1] * X[i-1];
		sum += b[2] * X[i-2];
		sum += b[3] * X[i-3];
		sum += b[4] * X[i-4];
		sum += b[5] * X[i-5];
		sum += b[6] * X[i-6];
		sum += b[7] * X[i-7];

		//Sum the outputs (unrolled)
		sum += a[0] * Y[i-1];
		sum += a[1] * Y[i-2];
		sum += a[2] * Y[i-3];
		sum += a[3] * Y[i-4];
		sum += a[4] * Y[i-5];
		sum += a[5] * Y[i-6];
		sum += a[6] * Y[i-7];
		sum += a[7] * Y[i-8];
		sum += a[8] * Y[i-9];
		sum += a[9] * Y[i-10];
		sum += a[10] * Y[i-11];
		sum += a[11] * Y[i-12];

		Y[i] = sum;
	}

	//Copy the last 8 inputs
	memcpy(memX, X + len - 8, 8 * sizeof(float));

	//Copy the last 12 outputs
	memcpy(memY, Y + len - 12, 12 * sizeof(float));
}


