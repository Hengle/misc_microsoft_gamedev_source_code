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

#include "AkCommon.h"
#include "AkSettings.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>

#ifndef _IMOTIONBUS_H
#define _IMOTIONBUS_H

//Maximum frames per buffer.  Derived from the maximum audio frames.
#ifdef RVL_OS
#define AK_FEEDBACK_MAX_FRAMES_PER_BUFFER 2
#else
#define AK_FEEDBACK_MAX_FRAMES_PER_BUFFER (AkUInt16)(AK_NUM_VOICE_REFILL_FRAMES * AK_FEEDBACK_SAMPLE_RATE / AK_CORE_SAMPLERATE)
#endif

//NOTE: This should change when Multi-Channel support is done for Audio.  We should use the same principle.
//#define MAX_FEEDBACK_CHANNELS 4
// Note: Consider allocating AkAudioMix dynamically (WG-9109).

class IAkMotionMixBus : public AK::IAkPlugin
{
public:
	virtual AKRESULT 	Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer)= 0;

	virtual AKRESULT	MixAudioBuffer( AkAudioBufferFinalMix &io_rBuffer ) = 0;
	virtual AKRESULT	MixFeedbackBuffer( AkAudioBufferFinalMix &io_rBuffer ) = 0;
	virtual AKRESULT	RenderData() = 0;
	virtual void		CommandTick() = 0;
	virtual void		Stop() = 0;

	virtual AkReal32	GetPeak() = 0;
	virtual bool		IsStarving() = 0;
	virtual bool		IsActive() = 0;
	virtual AkChannelMask GetMixingFormat() = 0;
	virtual void		SetMasterVolume(AkReal32 in_fVol) = 0;
};
#endif
