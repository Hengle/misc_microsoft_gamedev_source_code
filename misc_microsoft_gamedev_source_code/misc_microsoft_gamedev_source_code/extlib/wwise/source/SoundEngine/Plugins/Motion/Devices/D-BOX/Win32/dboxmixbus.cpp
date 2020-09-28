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
#include "DBoxMixBus.h"
#include "sink.h"
#include "VistaSink.h"

#define AK_IDX_SETUP_4_DBOX_FRONT_RIGHT		(0)
#define AK_IDX_SETUP_4_DBOX_FRONT_LEFT		(1)
#define AK_IDX_SETUP_4_DBOX_BACK_RIGHT		(2)
#define AK_IDX_SETUP_4_DBOX_BACK_LEFT		(3)

DBoxMixBus::DBoxMixBus() 
: m_oDecimator(AK_CORE_SAMPLERATE)
, m_oAccelFilter(AK_FEEDBACK_SAMPLE_RATE)
, m_pSink (NULL)
{
	memset(m_aCurrentMixBuffer, 0, sizeof(m_aCurrentMixBuffer));
	m_bGotData = false;

	m_oBuffer.AttachContiguousDeinterleavedData( 
		&m_aCurrentMixBuffer, 
		DBOX_NUM_REFILL_FRAMES,
		DBOX_NUM_REFILL_FRAMES,
		DBOX_CHANNEL_SETUP );

	m_fVolume = 1.0f;
}

DBoxMixBus::~DBoxMixBus()
{
}

AKRESULT DBoxMixBus::Init(AK::IAkPluginMemAlloc * in_pAllocator, AkPlatformInitSettings * io_pPDSettings, AkUInt8 in_iPlayer)
{	
	AKRESULT akr = AK_Fail;
	OSVERSIONINFO oWindowsVersion;
	oWindowsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&oWindowsVersion);
	if (oWindowsVersion.dwMajorVersion >= 6 /*Windows Vista*/)
	{
		m_pSink = AK_PLUGIN_NEW(in_pAllocator, DBoxSinkVista());
		if (m_pSink == NULL)
			return AK_Fail;

		akr = m_pSink->Init(io_pPDSettings->uNumRefillsInVoice);
	}
	else
	{
		DBoxSinkXP* pSink = AK_PLUGIN_NEW(in_pAllocator, DBoxSinkXP());
		if (pSink == NULL)
			return AK_Fail;

		akr = pSink->Init(io_pPDSettings->hWnd, io_pPDSettings->uNumRefillsInVoice);
		m_pSink = pSink;
	}

	if (akr != AK_Success && m_pSink != NULL)
	{
		m_pSink->Term(in_pAllocator);
		m_pSink = NULL;
	}
		
	return akr;
}

AKRESULT DBoxMixBus::Term(AK::IAkPluginMemAlloc * in_pAllocator)
{
	if (m_pSink != NULL)
	{
		m_pSink->Term(in_pAllocator);
		m_pSink = NULL;
	}

	AK_PLUGIN_DELETE(in_pAllocator, this);
	return AK_Success;
}

AKRESULT DBoxMixBus::Reset( )
{
	m_oDecimator.Reset();
	m_oAccelFilter.Reset();
	return AK_Success;
}

AKRESULT DBoxMixBus::GetPluginInfo( AkPluginInfo & out_rPluginInfo)
{
	return AK_NotImplemented;
}

//WARNING, this function should be called only ONCE per frame.
//Currently, the buffering assumes that there is only one audio buffer
//added to the mix per call to RenderData.
AKRESULT DBoxMixBus::MixAudioBuffer( AkAudioBufferFinalMix &io_rBuffer )
{	
	if (io_rBuffer.uValidFrames == 0)
		return AK_Success;	

	//Filter and decimate the input.  The work is done in-place.
	m_oDecimator.Execute(io_rBuffer);

	MixBuffer(io_rBuffer);
	
	return AK_Success;
}

AKRESULT DBoxMixBus::MixFeedbackBuffer(AkAudioBufferFinalMix &io_rBuffer)
{
	if (io_rBuffer.uValidFrames == 0)
		return AK_Success;	

	AKASSERT(io_rBuffer.GetChannelMask() == AK_SPEAKER_SETUP_4);

	//Filter the input to conform the signal to the specifications.  The work is done in-place.
	m_oAccelFilter.Execute(io_rBuffer);

	MixBuffer(io_rBuffer);
	
	return AK_Success;
}

void DBoxMixBus::MixBuffer(AkAudioBuffer &io_rBuffer)
{
	//The DBOX chair doesn't define its channels in the same order as the Sound Engine.
	//We must take care of the reordering while  interleaving.  There is no volume to account for,
	//the relative weight of all inputs was already computed in the previous mixing stage.

	//Assume that we have a multiple of 4 samples per buffer.  We will process 4 samples in one go.
	AKASSERT((io_rBuffer.uValidFrames & 3) == 0);
	AKASSERT(io_rBuffer.uValidFrames == AK_FEEDBACK_MAX_FRAMES_PER_BUFFER);

	__m128* AK_RESTRICT pSamples0 = (__m128*)io_rBuffer.GetChannel(0);
	__m128* AK_RESTRICT pSamples1 = (__m128*)io_rBuffer.GetChannel(1);
	__m128* AK_RESTRICT pSamples2 = (__m128*)io_rBuffer.GetChannel(2);
	__m128* AK_RESTRICT pSamples3 = (__m128*)io_rBuffer.GetChannel(3);
	__m128* AK_RESTRICT pDest = (__m128*)m_aCurrentMixBuffer;

	//Now process all channels
	__m128 mVol = _mm_load_ps1(&m_fVolume);
	__m128 mX0;	
	__m128 mX1;	
	__m128 mX2;	
	__m128 mX3;	
	for(AkUInt16 i = 0; i < io_rBuffer.uValidFrames; i += 4)
	{
		//Get 4 samples of each channel and interleave them
		//Reorder the channels (swap 0-1 and 2-3) and do the first half of interleaving
		mX0 = _mm_unpacklo_ps(*pSamples1, *pSamples0);	//Contains the first and second samples of channel FrontRight/FrontLeft
		mX2 = _mm_unpackhi_ps(*pSamples1, *pSamples0);	//Contains the third and fourth samples of channel FrontRight/FrontLeft
		pSamples0++;
		pSamples1++;

		mX1 = _mm_unpacklo_ps(*pSamples3, *pSamples2);	//Contains the first and second samples of channel BackRight/BackLeft
		mX3 = _mm_unpackhi_ps(*pSamples3, *pSamples2);	//Contains the third and fourth samples of channel BackRight/BackLeft
		pSamples2++;
		pSamples3++;

		//Mix and Store the first frame
		*pDest = _mm_add_ps(_mm_mul_ps(_mm_movelh_ps(mX0, mX1), mVol), *pDest);
		pDest++;

		//Mix and Store the second frame
		*pDest = _mm_add_ps(_mm_mul_ps(_mm_movehl_ps(mX1, mX0), mVol), *pDest);
		pDest++;

		//Mix and Store the third frame
		*pDest = _mm_add_ps(_mm_mul_ps(_mm_movelh_ps(mX2, mX3), mVol), *pDest);
		pDest++;

		//Mix and Store the fourth frame
		*pDest = _mm_add_ps(_mm_mul_ps(_mm_movehl_ps(mX3, mX2), mVol), *pDest);
		pDest++;
	}

	m_bGotData = true;
}

AKRESULT DBoxMixBus::RenderData() 
{
	AkUInt16 usEmptyBuffers = 0;
	AKRESULT akr = m_pSink->IsDataNeeded(usEmptyBuffers);
	if (usEmptyBuffers == 0 || akr != AK_Success)
		return akr;

	if(m_bGotData)
	{
		m_pSink->PassData(m_oBuffer);
		memset(m_aCurrentMixBuffer, 0, DBOX_NUM_REFILL_FRAMES*DBOX_CHANNELS*sizeof(AkReal32));
	}
	else
	{
		//Send silence (maintain the last level)
		m_pSink->PassSilence(DBOX_NUM_REFILL_FRAMES);
	}

	m_bGotData = false;

	return AK_Success;
}

void DBoxMixBus::CommandTick()
{
	return;
}

AkReal32 DBoxMixBus::GetPeak()
{
	return m_pSink->GetPeak();
}

bool DBoxMixBus::IsStarving()
{
	bool bStarved = m_pSink->IsStarved();
	m_pSink->ResetStarved();
	return bStarved;
}

bool DBoxMixBus::IsActive()
{
	return true;	//If we managed to initialize then, we are active.
}

AkChannelMask DBoxMixBus::GetMixingFormat()
{
	return AK_SPEAKER_SETUP_4;
}

void DBoxMixBus::Stop()
{
	m_pSink->Stop();
}

void DBoxMixBus::SetMasterVolume( AkReal32 in_fVol )
{
	m_fVolume = in_fVol;
}