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
#include "AkSettings.h"

#include "PlatformAudioLibDefs.h"

namespace AkAudioLibSettings
{
	AkUInt32 g_pipelineCoreFrequency = DEFAULT_NATIVE_FREQUENCY ;
	AkTimeMs g_msPerBufferTick = (AkUInt32)( LE_MAX_FRAMES_PER_BUFFER / ( DEFAULT_NATIVE_FREQUENCY / 1000.0f ) );
	AkUInt32 g_pcWaitTime = ((AkUInt32)( 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / DEFAULT_NATIVE_FREQUENCY / 2.0f ));
	AkUInt32 g_uLpfUpdatePeriod = DEFAULT_LPF_UPDATE_PERIOD;

	void SetSampleFrequency( AkUInt32 in_uSampleFrequency )
	{
		g_pipelineCoreFrequency = in_uSampleFrequency;
		g_msPerBufferTick = (AkUInt32)( LE_MAX_FRAMES_PER_BUFFER / ( in_uSampleFrequency / 1000.0f ) );
		g_pcWaitTime = ((AkUInt32)( 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / in_uSampleFrequency / 2.0f ));
		g_uLpfUpdatePeriod = (AkUInt32)( ( ((AkReal32)in_uSampleFrequency) / ((AkReal32)(DEFAULT_NATIVE_FREQUENCY)) ) * DEFAULT_LPF_UPDATE_PERIOD );
	}
};