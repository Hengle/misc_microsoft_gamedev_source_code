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
// sink.h
// Implementation of the sink.  This object manages the DirectSound
// AkInterface connected to the D-Box.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sink.h"
#include <dsound.h>
#include "..\Common\DBoxDefs.h"
#include <ks.h>
#include <ksmedia.h>

#define FRAME_SIZE (DBOX_CHANNELS*sizeof(DBOX_SAMPLE_TYPE))

GUID DBoxSinkXP::s_guidDBoxDevice = GUID_NULL;

extern void XPMotionDeviceInit(const WCHAR * sDeviceName);

class TraitXP
{
public:
	static inline void OutputOneFrame(const __m128 &in_rSamples, __m64 *in_pDest, AkUInt32& io_uOffset, AkUInt32 in_uOffsetMask)
	{
		__m64 mBlock = _mm_packs_pi32(_mm_cvtps_pi32(in_rSamples), _mm_cvtps_pi32(_mm_movehl_ps(in_rSamples, in_rSamples)));
		_mm_stream_pi(in_pDest + io_uOffset, mBlock);

		io_uOffset = (io_uOffset + 1) & in_uOffsetMask;
	}

	static inline void ScaleValue(__m128 &in_rSamples, __m128 &out_rScaled)
	{
		out_rScaled = _mm_mul_ps(in_rSamples, s_mMultiplier);
	}

	static const __m128 s_mMultiplier;

	typedef AkInt16 OutputType;
	typedef __m64 BlockType;
};

const __m128 TraitXP::s_mMultiplier = {AUDIOSAMPLE_INT_MAX, AUDIOSAMPLE_INT_MAX, AUDIOSAMPLE_INT_MAX, AUDIOSAMPLE_INT_MAX};

DBoxSinkXP::DBoxSinkXP()
{
	m_pvStart = NULL;				
	m_pvEnd = NULL;
	m_ulRefillOffset = 0;			
	m_uFreeRefillFrames = 0;
	m_uPlay = 0;	
	m_uWrite = 0;				
	m_ulBufferSize = 0;
	m_eState = StateSilence;

	ResetStarved();
}

DBoxSinkXP::~DBoxSinkXP()
{
}

/****************************************************************
* Init
* Initialize the direct sound AkInterface
* Params: 
* HWND in_hwnd : Window receiving the DX messages
* AkUInt32 in_iRefillBuffers: Number of buffers to allocate
****************************************************************/
AKRESULT DBoxSinkXP::Init(HWND in_hwnd, AkUInt32 in_iRefillBuffers)
{
	//DBox is a USB Audio device.  Enumerate all audio devices and find the one.
	if (s_guidDBoxDevice == GUID_NULL)
	{
		DirectSoundEnumerate((LPDSENUMCALLBACK)EnumCallback, NULL);
		if (s_guidDBoxDevice == GUID_NULL)
			return AK_Fail;
	}

	HRESULT hr;
	hr = DirectSoundCreate8(&s_guidDBoxDevice, &m_spDirectSound, NULL);
	if (FAILED(hr))
		return AK_Fail;

	XPMotionDeviceInit(DBOX_DRIVER_NAME);

	if( FAILED(m_spDirectSound->SetCooperativeLevel(in_hwnd, DSSCL_PRIORITY)) )
	{
		AKASSERT(!"Unable to set cooperative level");
		return AK_Fail;
	}

	DSBUFFERDESC	dsbd;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

	if( FAILED(m_spDirectSound->CreateSoundBuffer(&dsbd, &m_spPrimaryBuffer, NULL)) )
	{
		AKASSERT(!"Unable to create the primary buffer");
		return AK_Fail;
	}

	//Ensure the buffer size is a multiple of an output sample to avoid dealing with partial upsampled samples.
	//DBOX_NUM_REFILL_FRAMES is the max number of frames in one buffer.
	//FRAME_SIZE is the size of one interleaved sample, in bytes
	//This should be an integer!
	AKASSERT((BUFFER_COUNT * DBOX_CHANNELS * DBOX_NUM_REFILL_FRAMES * DBOX_OUTPUT_RATE) % DBOX_SAMPLERATE == 0);

	AkUInt32 iFrames = (AkUInt32)ceilf(BUFFER_COUNT * DBOX_NUM_OUTPUT_FRAMES);
	m_ulBufferSize = iFrames * FRAME_SIZE;

	CSinkBase::Init(iFrames);

	//Ensure that the buffer size is a power of two.  We make this assumption everywhere.
#ifdef _DEBUG
	AkUInt32 iCount = 0;
	for(AkUInt32 iBit = 0; iBit < 32; iBit++)
		iCount += (m_ulBufferSize >> iBit) & 1;
	AKASSERT(iCount == 1);
#endif
	m_ulBufferMask = m_ulBufferSize - 1;

	return SetPrimaryBufferFormat();
}

/****************************************************************
* Term
* Terminates the direct sound Interface
****************************************************************/
void DBoxSinkXP::Term(AK::IAkPluginMemAlloc * in_pAllocator)
{
	//Stop should have been called.
	AKASSERT(m_spDSBuffer == NULL);
	AKASSERT(m_spPrimaryBuffer == NULL);

	m_spDirectSound = NULL;

	m_pvStart = NULL;		
	m_pvEnd = NULL;
	m_ulRefillOffset = 0;			
	m_uFreeRefillFrames = 0;
	m_uPlay = 0;	
	m_uWrite = 0;				
	m_ulBufferSize = 0;

	CSinkBase::Term(in_pAllocator);
	AK_PLUGIN_DELETE(in_pAllocator, this);
}

void DBoxSinkXP::PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt )
{
	memset( &out_wfExt, 0, sizeof( WAVEFORMATEXTENSIBLE ) );

	out_wfExt.Format.nChannels			= DBOX_CHANNELS;
	out_wfExt.Format.nSamplesPerSec		= DBOX_OUTPUT_RATE;
	out_wfExt.Format.wBitsPerSample		= DBOX_BITS_PER_SAMPLE;
	out_wfExt.Format.nBlockAlign		= sizeof(DBOX_SAMPLE_TYPE) * DBOX_CHANNELS;
	out_wfExt.Format.nAvgBytesPerSec	= DBOX_OUTPUT_RATE * out_wfExt.Format.nBlockAlign;

	out_wfExt.Format.cbSize		= sizeof( WAVEFORMATEXTENSIBLE ) - sizeof( WAVEFORMATEX );
	out_wfExt.Format.wFormatTag	= WAVE_FORMAT_EXTENSIBLE;

	out_wfExt.dwChannelMask					= SPEAKER_FRONT_LEFT |
		SPEAKER_FRONT_RIGHT | 
		SPEAKER_BACK_LEFT |
		SPEAKER_BACK_RIGHT;

	out_wfExt.SubFormat						= KSDATAFORMAT_SUBTYPE_PCM;
	out_wfExt.Samples.wValidBitsPerSample	= out_wfExt.Format.wBitsPerSample;
}

/****************************************************************
* Start
* Starts the looping playback
****************************************************************/
AKRESULT DBoxSinkXP::Start()
{
	// temporary one
	CAkSmartPtr<IDirectSoundBuffer> spDSB = NULL;

	// Create a DSBuffer to play this sound
	WAVEFORMATEXTENSIBLE wfExt;
	DSBUFFERDESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME |DSBCAPS_GLOBALFOCUS;
	desc.lpwfxFormat = (WAVEFORMATEX *) &wfExt;
	desc.dwBufferBytes = m_ulBufferSize;

	PrepareFormat( wfExt );

	//Create the DS secondary buffer
	HRESULT hr = m_spDirectSound->CreateSoundBuffer(&desc, &spDSB, NULL);
	if (FAILED(hr))
	{
		AKASSERT(!"Could not create the secondary buffer");
		return AK_CouldNotCreateSecBuffer;
	}

	void*		pData1 = NULL;
	AkUInt32	ulData1Bytes;
	void*		pData2 = NULL;
	AkUInt32	ulData2Bytes;

	//Get the AkInterface to our secondary buffer
	CAkSmartPtr<IDirectSoundBuffer8> spBuffer;
	hr = spDSB->QueryInterface(IID_IDirectSoundBuffer8, (void**)&spBuffer);
	if (FAILED(hr))
	{
		AKASSERT(!"Could not get DS buffer");
		return AK_Fail;
	}

	// figure out where the buffer is in memory
	hr = spBuffer->Lock(0, m_ulBufferSize,
		&pData1, &ulData1Bytes,
		&pData2, &ulData2Bytes,
		DSBLOCK_FROMWRITECURSOR);
	if (FAILED(hr))
	{
		AKASSERT(!"Could not lock buffer");
		return AK_Fail;
	}

	// save the starting address
	m_pvStart = (AkInt8*)pData1;
	m_pvEnd = m_pvStart + m_ulBufferSize;

	::ZeroMemory( m_pvStart, ulData1Bytes );

	hr = spBuffer->Unlock(pData1, ulData1Bytes, pData2, ulData2Bytes);
	if (FAILED(hr))
	{
		AKASSERT(!"Could not unlock buffer");
		return AK_Fail;
	}

	spBuffer->SetVolume(0);
	spBuffer->SetFrequency(DBOX_OUTPUT_RATE);

	hr = spBuffer->Play(0,0,DSBPLAY_LOOPING);
	if (FAILED(hr))
	{
		AKASSERT(!"Could not start playback");
		return AK_Fail;
	}

	m_spDSBuffer = spBuffer;

	//Start with the buffer half full (of silence), so the play head can start playing "real" data (data we know).
	m_ulRefillOffset = (AkUInt16)(DBOX_NUM_OUTPUT_FRAMES) * (BUFFER_COUNT/2) * FRAME_SIZE;

	return AK_Success;
}

/****************************************************************
* EnumCallback
* Called by DirectSoundEnumerate for each device.  Tries to find 
* the D-Box device.
* Params: See MSDN.
****************************************************************/
BOOL CALLBACK DBoxSinkXP::EnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	//Weird, the doc says its a char*, the signature says its a char*.  But its a wchar*.  Go figure.
	if (wcsstr((WCHAR*)lpcstrDescription, DBOX_DRIVER_NAME) != NULL)
	{
		s_guidDBoxDevice = *lpGuid;
		return FALSE; //Stop enumeration.
	}
	return TRUE;
}

/****************************************************************
* SetPrimaryBufferFormat
* Initializes the primary buffer.
****************************************************************/
AKRESULT DBoxSinkXP::SetPrimaryBufferFormat()
{
	AKASSERT(m_spPrimaryBuffer != NULL);
	m_spPrimaryBuffer->Stop();

	WAVEFORMATEXTENSIBLE wfExt;
	PrepareFormat( wfExt );

	if(FAILED(m_spPrimaryBuffer->SetFormat( (WAVEFORMATEX *) &wfExt )))
	{
		AKASSERT(!"Unable to set Primary Buffer format");
		return AK_Fail;
	}
	if(FAILED(m_spPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING)))
	{
		AKASSERT(!"Unable to play Primary Buffer");
		return AK_Fail;
	}
	return Start();
}

/****************************************************************
* GetBuffer
* Locks the playback buffer
****************************************************************/
AKRESULT DBoxSinkXP::GetBuffer(BufferUpdateParams& io_Params)
{
	HRESULT hResult = m_spDSBuffer->Lock(io_Params.ulOffset,io_Params.ulBytes,
		&io_Params.pvAudioPtr1,&io_Params.ulAudioBytes1,
		&io_Params.pvAudioPtr2,&io_Params.ulAudioBytes2,
		io_Params.ulFlags);

	if(FAILED(hResult))
		return AK_Fail;

	return AK_Success;
}

/****************************************************************
* ReleaseBuffer
* Unlocks the playback buffer
****************************************************************/
AKRESULT DBoxSinkXP::ReleaseBuffer(BufferUpdateParams& in_rParams)
{
	HRESULT hResult = m_spDSBuffer->Unlock(in_rParams.pvAudioPtr1,
		in_rParams.ulAudioBytes1,
		in_rParams.pvAudioPtr2,
		in_rParams.ulAudioBytes2);
	if(FAILED(hResult))
		return AK_Fail;

	return AK_Success;
}

/****************************************************************
* PassData
* Manage the wrap around of the ring buffer.
* Params:
* AkFeedbackBuffer& io_rXfer: Data buffer to send
****************************************************************/

AKRESULT DBoxSinkXP::PassData(AkPipelineBuffer& io_rXfer)
{
	if ( m_spDSBuffer == NULL ) 
		return AK_Fail;

	if(io_rXfer.uValidFrames == 0)
		return AK_Success;

	m_eState = StatePlay;

	BufferUpdateParams	Params;
	memset(&Params,0,sizeof(Params));
	Params.ulFlags = DSBLOCK_ENTIREBUFFER;
	Params.ulBytes = m_ulBufferSize;

	// lock the buffer before trying to write there
	AKRESULT eResult = AK_Success;
	eResult = GetBuffer(Params);
	if(eResult != AK_Success)
		return eResult;

	//Limit the signal within the device parameters.
	m_oLimiter.Limit((AkReal32*)io_rXfer.GetInterleavedData(), io_rXfer.MaxFrames(), m_Output1, m_Output2);

	AkReal32* pData = (AkReal32*)io_rXfer.GetInterleavedData();
	Fill<TraitXP>(pData, io_rXfer.uValidFrames, (AkInt16*)m_pvStart, m_ulRefillOffset);

	// try to release the buffer
	return ReleaseBuffer(Params);
}

/****************************************************************
* PassSilence
* Send silence to the device
* Params:
* AkUInt32 uNumFrames: Number of frames of silence
****************************************************************/
AKRESULT DBoxSinkXP::PassSilence( AkUInt16 in_uNumFrames )
{
	if ( m_spDSBuffer == NULL ) 
		return AK_Fail;

	if(in_uNumFrames == 0)
		return AK_Success;

	AKRESULT eResult = AK_Success;


	if (m_eState == StatePlay)
	{
		m_ulSilenceSamples = 0;
		m_eState = StateSilence;
	}

	if(m_eState == StateSilence)
	{
		BufferUpdateParams	Params;
		memset(&Params,0,sizeof(Params));
		Params.ulFlags = DSBLOCK_ENTIREBUFFER;
		Params.ulBytes = m_ulBufferSize;

		// lock the buffer before trying to write there
		eResult = GetBuffer(Params);
		if(eResult != AK_Success)
			return eResult;

		m_ulSilenceSamples += FillSilence<TraitXP>((AkInt16*)m_pvStart, in_uNumFrames, m_ulRefillOffset);

		//If all samples are silence, avoid moving around data that already exist.
		if (m_ulSilenceSamples >= m_ulBufferSize / FRAME_SIZE)
			m_eState = StateEconoSilence;

		eResult = ReleaseBuffer(Params);
	}
	else
	{
		//Only advance the pointers
		AkUInt16 lBufferFrames = (AkUInt16)(in_uNumFrames * SAMPLE_MULTIPLIER);

		//Maintain the cycle state manually also.
		m_iExtraState += (m_iExtraState + in_uNumFrames) % 3;
		if (m_iExtraState >= 3)
		{
			lBufferFrames++;
			m_iExtraState -= 3;
		}

		m_ulRefillOffset += lBufferFrames * FRAME_SIZE;
		m_ulRefillOffset &= m_ulBufferMask;
	}

	// try to release the buffer
	return eResult;
}

/****************************************************************
* IsDataNeeded
* Check how many buffers (samples) are needed to continue playback.
* Params:
* AkUInt32 & out_uBuffersNeeded : Number of buffers.
****************************************************************/
AKRESULT DBoxSinkXP::IsDataNeeded( AkUInt16 & out_uBuffersNeeded )
{
	AKRESULT eResult = GetRefillSize( m_uFreeRefillFrames );
	if (!(eResult == AK_Success || eResult == AK_PartialSuccess))
		return eResult;

	out_uBuffersNeeded = (AkUInt16)(m_uFreeRefillFrames / DBOX_NUM_OUTPUT_FRAMES);

	return eResult;
}

/****************************************************************
* GetRefillSize
* Check how many samples are needed to avoid starving.
* Params:
* AkUInt32& out_uRefillFrames: Samples needed to fill up buffer.
****************************************************************/
AKRESULT DBoxSinkXP::GetRefillSize( AkUInt32& out_uRefillFrames )
{
	if ( m_spDSBuffer == NULL ) 
		return AK_Fail;

	// figure out where the play position is
	AkUInt32 uOldPlay = m_uPlay;
	HRESULT hr = m_spDSBuffer->GetCurrentPosition(&m_uPlay, &m_uWrite);
	if ( hr != DS_OK )
		return AK_Fail;

	// handle cases where play and write are 0
	if((m_uPlay + m_uWrite) == 0)
	{
		// return a number of frames
		out_uRefillFrames = m_ulBufferSize / FRAME_SIZE;
		return AK_Success;
	}

	//Check 3 cases for starvation
	//OP -> Old Play head
	//OR -> Old Refill position
	//P -> Current Playhead
	//+------OP---OR---P------+
	//+---P------OP---OR-----+
	//+--OR------P------OP---+
	m_bStarved = (uOldPlay < m_ulRefillOffset && m_ulRefillOffset < m_uPlay) ||
		(m_uPlay < uOldPlay && uOldPlay < m_ulRefillOffset) ||
		(m_ulRefillOffset < m_uPlay && m_uPlay < uOldPlay);

	AkUInt32 ulFreeRefillSize = 0;
	if(m_ulRefillOffset > m_uPlay)
	{
		ulFreeRefillSize = m_ulBufferSize - m_ulRefillOffset + m_uPlay;
	}
	else
	{
		ulFreeRefillSize = m_uPlay - m_ulRefillOffset;
	}

	// return a number of frames
	out_uRefillFrames = (AkUInt32)ulFreeRefillSize / FRAME_SIZE;

	AkInt16 iCorrection = ManageDrift(out_uRefillFrames);

	//In economic silence, we must maintain the offset manually.  Beware, the correction can be negative.
	if (m_eState == StateEconoSilence && (AkInt16)m_ulRefillOffset >= iCorrection)
	{
		m_ulRefillOffset += iCorrection;
		m_ulRefillOffset &= m_ulBufferMask;
	}

	return AK_Success;
}

void DBoxSinkXP::Stop()
{
	if (m_spDSBuffer != NULL)
		m_spDSBuffer->Stop();

	if (m_spPrimaryBuffer != NULL)
		m_spPrimaryBuffer->Stop();

	m_spDSBuffer = NULL;
	m_spPrimaryBuffer = NULL;
}