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


//Implementation of a 7th order IIR filter

class CIIRFilter
{
public:
	CIIRFilter();
	~CIIRFilter();

	void Execute(float *X, float *Y, unsigned int len);
	void Reset();

private:	
	float memX[8];	 //Memory of the input
	float memY[12];	 //Memory of the output
};