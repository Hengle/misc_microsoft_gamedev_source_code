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

////////////////////////////////////////////////////////////////////////
// RumbleBus.cpp
//
// Final mix bus for the rumble function of the game controllers.
//
// Copyright 2007 Audiokinetic Inc.
//
// Author:  mjean
// Version: 1.0
//
///////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RumbleBus.h"
#include "math.h"
#include <AkSettings.h>

#ifdef XBOX360
#include <Xffb.h>
#endif

RumbleMixBus::RumbleMixBus()
{
	memset(m_pData, 0, sizeof(AkReal32) * BUFFER_SIZE);
	memset(&m_oCurrent, 0, sizeof(m_oCurrent));
	m_usWriteBuffer = 0;
	m_usReadBuffer = 0;
	m_fPeak = 0;
	m_iPlayer = 0;
	m_bStopped = true;
	m_bGotData = false;
	m_iDrift = 0;
	m_fVolume = 1.0f;

#if defined( RVL_OS ) || defined( AK_PS3 )
	m_fAverageSpeed = 0;
#endif
}

RumbleMixBus::~RumbleMixBus()
{
}

AKRESULT RumbleMixBus::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	SendSample(0,0);
	AK_PLUGIN_DELETE(in_pAllocator, this);
	return AK_Success;
}

AKRESULT RumbleMixBus::Reset()
{
	return AK_Success;
}

AKRESULT RumbleMixBus::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	return AK_NotImplemented;
}

AKRESULT RumbleMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer)
{
	m_iPlayer = in_iPlayer;

#ifdef RVL_OS	//Wii
	WPADStatus oStatus;
	const s32 wpadStatus = WPADGetStatus();
	if ( wpadStatus != WPAD_STATE_SETUP )
	{
		OSReport( "Rumble: Note: Wwimote rumble support will not be available because WPadInit() was not called before initializing the Wwise sound engine\n" );
		return AK_Fail;
	}
#endif

	memset(&m_oCurrent, 0, sizeof(m_oCurrent));

	return AK_Success;
}

AKRESULT RumbleMixBus::MixAudioBuffer( AkAudioBufferFinalMix &io_rBuffer )
{
	//Don't process the LFE
	AkUInt32 uNumChannel = AK::GetNumChannels(io_rBuffer.GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY);
	for (AkUInt16 iSample = 0; iSample < SAMPLE_COUNT; ++iSample)
	{
		AkReal32 fMax = 0;
		AkUInt16 iSamplesToProcess = io_rBuffer.uValidFrames / SAMPLE_COUNT;

		for(AkUInt32 iChannel = 0; iChannel < uNumChannel; ++iChannel)
		{
			AkReal32 *pData = (AkReal32*)io_rBuffer.GetChannel(iChannel) + iSample * iSamplesToProcess;
			for(AkUInt16 i = 0; i < iSamplesToProcess; ++i)
			{
				fMax = AkMax(*pData, fMax);
				pData++;
			}
		}

		m_pData[m_usWriteBuffer + iSample * CHANNEL_COUNT] += fMax * m_fVolume;		//Left motor
		m_pData[m_usWriteBuffer + iSample * CHANNEL_COUNT + 1] += fMax * m_fVolume;	//Right motor
	}

	m_bGotData = true;

	return AK_Success;
}

AKRESULT RumbleMixBus::MixFeedbackBuffer( AkAudioBufferFinalMix &io_rBuffer )
{
	if (io_rBuffer.uValidFrames == 0)
		return AK_Success;

	//Grab only the two samples, the first one and the middle one
	AkReal32 * pData;
	for(AkUInt16 i = 0; i < CHANNEL_COUNT; i++)
	{
		pData = (AkReal32*)io_rBuffer.GetChannel(i);

		//Interleave the samples in the final buffer
		m_pData[m_usWriteBuffer + i] += pData[0] * m_fVolume;
		m_pData[m_usWriteBuffer + i + SAMPLE_COUNT] += pData[io_rBuffer.MaxFrames() / 2] * m_fVolume;
	}
	m_bGotData = true;
	return AK_Success;
}

AKRESULT RumbleMixBus::RenderData()
{
	AkUInt16 i = 0;
	if (!m_bGotData)
	{
		if(!m_bStopped)
		{
			//Put zeros to send next.
			for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
				m_pData[m_usWriteBuffer + i] = 0.0f;

			//Advance the write pointer
			m_usWriteBuffer = (m_usWriteBuffer + SAMPLE_COUNT*CHANNEL_COUNT) & BUFFER_MASK;

			//Clear the buffer for the next mix.
			for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
				m_pData[m_usWriteBuffer + i] = 0.0f;

			m_iDrift = 0;
		}
		return AK_Success; 
	}

	for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
	{
		//Compute peak
		m_fPeak = AkMax(m_pData[m_usWriteBuffer+i ], m_fPeak);
	}

	//Advance the write pointer
	m_usWriteBuffer = (m_usWriteBuffer + SAMPLE_COUNT*CHANNEL_COUNT) & BUFFER_MASK;

	for(i = 0; i < SAMPLE_COUNT*CHANNEL_COUNT; i++)
	{
		//Clear the buffer for the next mix.
		m_pData[m_usWriteBuffer + i] = 0.0f;
	}

	m_bGotData = false;

	//Added 2 samples
	m_iDrift += 2;

	//Because we don't really control the number of buffers processed per slice (we're piggy-backing the
	//audio thread's clock), we need to make sure we don't lag behind too much.  It is normal that audio buffer
	//be processed 2 or 3 in a row.  It is possible that we receive only one clock tick during that time.
	//Our clock ticks are variable because of this which induces a lag.  Fortunately, it won't be felt if we 
	//skip a sample once in a while.  We just need to make sure the write head don't overwrite the read head.
	if(m_iDrift > SAMPLE_COUNT*BUFFER_COUNT / 2)
	{
		m_usReadBuffer = (m_usReadBuffer + SAMPLE_COUNT) & BUFFER_MASK;
		m_iDrift -= 1;
	}

	return AK_Success;
}

void RumbleMixBus::CommandTick()
{
	//Don't send anything if there is nothing in the buffer.  We don't care about starvation.
	if (m_usReadBuffer == m_usWriteBuffer)
	{
		m_iDrift = 0;
		return;
	}

	//Send the next sample
	SendSample(m_pData[m_usReadBuffer], m_pData[m_usReadBuffer + 1]);
	m_usReadBuffer = (m_usReadBuffer + CHANNEL_COUNT) & BUFFER_MASK;

	//Sent one sample
	m_iDrift--;
}

void RumbleMixBus::SendSample(AkReal32 in_fLarge, AkReal32 in_fSmall)
{
	m_bStopped = (in_fLarge + in_fSmall) == 0.0f;
	if (in_fLarge > 1.0f)
		in_fLarge = 1.0f;
		
	if (in_fSmall > 1.0f)
		in_fSmall = 1.0f;


#ifdef AK_DIRECTX

	//For the XBox controller, the response is not linear.  It goes from 0 to -20 dB (approximately) following
	//a kind of inverse exponential.  We will linearize the response.  To simplify the maths, we will simply
	//use 2 linear interpolation with different slopes.  The corner of the 2 slopes was determined experimentally.
	const AkReal32 fRange = 65535.f;
	const AkReal32 fLimit = 0.08f * fRange;	//Below 10% the motor doesn't turn.
	const AkReal32 fCornerX = 0.4f;
	const AkReal32 fCornerY = 0.15f * fRange;
	const AkReal32 fFirstSlope = (fRange-fCornerY - 2*fLimit)/(1-fCornerX);
	const AkReal32 fFirstOffset = fRange-fFirstSlope;
	const AkReal32 fSecondSlope = (fCornerY-fLimit)/fCornerX;
	
	AkUInt16 usLarge = 0;
	if (in_fLarge > fCornerX)
		usLarge = (AkUInt16)(in_fLarge * fFirstSlope + fFirstOffset);
	else if (in_fLarge > 0.05 )
		usLarge = (AkUInt16)(in_fLarge * fSecondSlope + fLimit);

	//These values were obtained through experimentation.  Below 5%, there is nothing
	//happening.  So the interpolation starts at that value.  
	//We want the full scale applied to the real range of values.
	AkUInt16 usSmall = 0;
	if (in_fSmall > 0.05)
		usSmall = AkUInt16(in_fSmall * (65535.0f -8192.0f) + 8192.0f);

	if (m_oCurrent.wLeftMotorSpeed != usLarge || m_oCurrent.wRightMotorSpeed != usSmall)
	{
		//We need to change the speed. (No overlapped IO for now)
		m_oCurrent.wLeftMotorSpeed = usLarge;
		m_oCurrent.wRightMotorSpeed = usSmall;
		XInputSetState(m_iPlayer, &m_oCurrent);
	}
#endif

#ifdef AK_PS3
	//These values were obtained through experimentation.  Below 40, there is nothing
	//happening.  So the interpolation starts at that value.  
	//We want the full scale applied to the real range of values.
	AkUInt8 usLarge = 0;
	if (in_fLarge > 0.05)
		usLarge = (AkUInt8)(in_fLarge * (255.5f - 40.0f)) + 40.0f;

	if ( PulseMod<uint8_t>( in_fSmall, 10.0f, 0, 1, m_oCurrent.motor[0] )
		|| m_oCurrent.motor[1] != usLarge)
	{
		//We need to change the speed. 
		m_oCurrent.motor[1] = usLarge;
		cellPadSetActDirect(m_iPlayer, &m_oCurrent);
	}
#endif

#ifdef RVL_OS
	if ( PulseMod<AkUInt32>( in_fLarge, 10.0f, WPAD_MOTOR_STOP, WPAD_MOTOR_RUMBLE, m_oCurrent ) )
		WPADControlMotor(m_iPlayer, m_oCurrent);
#endif
}

AkReal32 RumbleMixBus::GetPeak()
{
	AkReal32 fReturned = m_fPeak;
	m_fPeak = 0.0f;
	return fReturned;
}

bool RumbleMixBus::IsStarving()
{
	return false;
}

bool RumbleMixBus::IsActive()
{
#ifdef AK_DIRECTX
	DWORD err;
	XINPUT_CAPABILITIES oCap;
	err = XInputGetCapabilities(m_iPlayer, XINPUT_FLAG_GAMEPAD, &oCap);
	if (err != ERROR_SUCCESS || (oCap.Vibration.wLeftMotorSpeed == 0 && oCap.Vibration.wRightMotorSpeed == 0))
		return false;
#endif

#ifdef AK_PS3
	CellPadCapabilityInfo oInfo;
	cellPadGetCapabilityInfo(m_iPlayer, &oInfo);
	if ((oInfo.info[0] & CELL_PAD_CAPABILITY_ACTUATOR) == 0)
		return false;
#endif

#ifdef RVL_OS	//Wii
	WPADStatus oStatus;
	WPADRead(m_iPlayer, &oStatus);
	if (oStatus.err != WPAD_ERR_NONE)
		return false;
#endif

	return true;
}

AkChannelMask RumbleMixBus::GetMixingFormat()
{
	return AK_SPEAKER_SETUP_STEREO;
}

void RumbleMixBus::SetMasterVolume( AkReal32 in_fVol )
{
	m_fVolume = in_fVol;
}