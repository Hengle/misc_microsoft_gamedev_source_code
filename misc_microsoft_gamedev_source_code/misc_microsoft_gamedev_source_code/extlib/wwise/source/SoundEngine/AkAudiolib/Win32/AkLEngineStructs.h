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

#ifndef _AK_LENGINE_STRUCTS_H_
#define _AK_LENGINE_STRUCTS_H_

#include "AkCommon.h"
#include "AkFeedbackStructs.h"

class AkVPL;
class AkVPLSrc;

struct AkMergedEnvironmentValue
{
	AkEnvironmentValue envValue;
	float fLastControlValue;
};

//This structure contains the minimal amount of data needed to do a mix.
struct AkVPLMixState
{
	AkAudioBufferCbx buffer;
	AKRESULT	  result;
};

struct AkVPLState : public AkVPLMixState
{
	bool          bIsEnvironmentalBus;
	bool		  bAudible;
	bool		  bPause; // VPL needs to pause at end of this frame
	bool		  bStop; // VPL needs to stop at end of this frame

	AkMergedEnvironmentValue aMergedValues[ AK_MAX_ENVIRONMENTS_PER_OBJ ];
};

struct AkFeedbackVPLData
{
	AkPipelineBufferBase LPFBuffer;			//Buffer for LPF processing.
};

struct AkRunningVPL
{
	AkRunningVPL * pNextItem; // for AkListRunningVPL

	AkVPLSrc * pSrc;
	AkVPL *    pBus;
	AkVPLState state;

	AkFeedbackVPLData*	pFeedbackData;	//For Audio-To-Motion pipeline only.
	bool				bFeedbackVPL;	//Is it for the feedback pipeline or audio pipeline
};

struct AkSinkStats
{
	AkReal32 m_fOutMin;				// low peak of output
	AkReal32 m_fOutMax;				// high peak of output
	AkReal32 m_fOutSum;				// sum of output samples
	AkReal32 m_fOutSumOfSquares;	// sum of output samples^2
	AkUInt32 m_uOutNum;				// number of output samples
};

#endif // _AK_LENGINE_STRUCTS_H_
