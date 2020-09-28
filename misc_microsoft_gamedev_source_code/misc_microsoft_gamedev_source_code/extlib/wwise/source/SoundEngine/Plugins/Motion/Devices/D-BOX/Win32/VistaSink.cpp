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
// VistaSink.h
// Implementation of the sink.  This object manages the WASAPI
// interface connected to the D-Box.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VistaSink.h"
#include <AkSmartPtr.h>

//This defines comes from a Microsoft Sample.  To convert REFERENCE_TIME in milli seconds.
#define MFTIMES_PER_MILLISEC  10000

//Number of buffers.  We need more than 4.  With 6 the number of samples in the buffer is a power of 2 (1024 samples).
#define BUFFER_COUNT 6

extern void VistaMotionDeviceInit(const WCHAR * sDeviceName);

class TraitVista
{
public:

	static inline void OutputOneFrame(const __m128 &in_rSamples, __m128 *in_pDest, AkUInt32& io_uOffset, AkUInt32 in_uOffsetMask)
	{
		//Unfortunately the buffer given by WASAPI on Vista isn't necessary aligned.  
		_mm_storeu_ps((float*)(in_pDest + io_uOffset) , in_rSamples);

		io_uOffset++;	// We don't need to take care of the wrap around.
	}
	static inline void ScaleValue(__m128 &in_rSamples, __m128 &out_rScaled)
	{
		out_rScaled = in_rSamples;
	}

	typedef AkReal32 OutputType;
	typedef __m128 BlockType;
};

DBoxSinkVista::DBoxSinkVista()
: m_uBufferFrames( 0 )
{
}

DBoxSinkVista::~DBoxSinkVista()
{
}

AKRESULT DBoxSinkVista::Init(AkUInt32 in_iBufferFrames)
{
	CAkSmartPtr<IMMDeviceEnumerator> spEnumerator = NULL;
	CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&spEnumerator);

	if ( spEnumerator == NULL )
		return AK_Fail;

	//Find the DBOX device.
	CAkSmartPtr<IMMDeviceCollection> spCollection;
	HRESULT hr = spEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &spCollection);
	if (hr != S_OK)
		return AK_Fail;

	UINT dwCount = 0;
	spCollection->GetCount(&dwCount);
	for(AkUInt32 i = 0; i < dwCount; i++)
	{
		LPWSTR szId = NULL;
		CAkSmartPtr<IMMDevice> spDevice;
		spCollection->Item(i, &spDevice);

		CAkSmartPtr<IPropertyStore> spPropStore;
		spDevice->OpenPropertyStore(STGM_READ, &spPropStore);

		PROPVARIANT varName;
		hr = spPropStore->GetValue(PKEY_DeviceInterface_FriendlyName, &varName);
		if (hr == S_OK && wcsstr(varName.pwszVal, DBOX_DRIVER_NAME) != 0)
		{
			m_spDeviceOut = spDevice;
			CoTaskMemFree(varName.pwszVal);
			break;
		}
		CoTaskMemFree(varName.pwszVal);
	}

	if (m_spDeviceOut == NULL)
		return AK_Fail;

	hr = m_spDeviceOut->Activate( __uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**) &m_spClientOut );
	if ( hr != S_OK )
		return AK_Fail;

	VistaMotionDeviceInit(DBOX_DRIVER_NAME);

	WAVEFORMATEXTENSIBLE wfExt;
	PrepareFormat(wfExt);

	WAVEFORMATEX * pWfSupported = NULL;
	hr = m_spClientOut->IsFormatSupported( AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &wfExt, &pWfSupported );

	if ( hr != S_OK ) 
		return AK_Fail;	//MUST be supported

	CoTaskMemFree(pWfSupported);

	//Compute the time duration of our buffer.  We want BUFFER_COUNT buffers.
	REFERENCE_TIME timeBuffer = (REFERENCE_TIME)DBOX_NUM_OUTPUT_FRAMES * BUFFER_COUNT * MFTIMES_PER_MILLISEC * 1000 / DBOX_OUTPUT_RATE;

	hr = m_spClientOut->Initialize( AUDCLNT_SHAREMODE_SHARED, 0, timeBuffer, 0, (WAVEFORMATEX *) &wfExt, NULL );
	if ( hr != S_OK )
		return AK_Fail;

	hr = m_spClientOut->GetBufferSize( (UINT32*)&m_uBufferFrames );
	if ( hr != S_OK )
		return AK_Fail;

	hr = m_spClientOut->GetService( __uuidof(IAudioRenderClient), (void **) &m_spRenderClient );
	if ( hr != S_OK )
		return AK_Fail;

	if (m_spClientOut->Start() != S_OK)
		return AK_Fail;

	CSinkBase::Init(m_uBufferFrames);

	//Fill the buffer halfway.
	PassSilence(DBOX_NUM_REFILL_FRAMES * BUFFER_COUNT / 2);

	return AK_Success;
}

void DBoxSinkVista::PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt )
{
	memset( &out_wfExt, 0, sizeof( WAVEFORMATEXTENSIBLE ) );

	out_wfExt.Format.nChannels			= DBOX_CHANNELS;
	out_wfExt.Format.nSamplesPerSec		= DBOX_OUTPUT_RATE;
	out_wfExt.Format.wBitsPerSample		= sizeof(AkReal32)*8;	//Always float on Vista
	out_wfExt.Format.nBlockAlign		= sizeof(AkReal32) * DBOX_CHANNELS;
	out_wfExt.Format.nAvgBytesPerSec	= DBOX_OUTPUT_RATE * out_wfExt.Format.nBlockAlign;

	out_wfExt.Format.cbSize				= sizeof( WAVEFORMATEXTENSIBLE ) - sizeof( WAVEFORMATEX );
	out_wfExt.Format.wFormatTag			= WAVE_FORMAT_EXTENSIBLE;

	out_wfExt.dwChannelMask					= SPEAKER_FRONT_LEFT |
		SPEAKER_FRONT_RIGHT | 
		SPEAKER_BACK_LEFT |
		SPEAKER_BACK_RIGHT;

	out_wfExt.SubFormat						= KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	out_wfExt.Samples.wValidBitsPerSample	= out_wfExt.Format.wBitsPerSample;
}

void DBoxSinkVista::Term(AK::IAkPluginMemAlloc * in_pAllocator)
{
	m_spRenderClient = NULL;
	m_spClientOut = NULL;
	m_spDeviceOut = NULL;

	CSinkBase::Term(in_pAllocator);
	AK_PLUGIN_DELETE(in_pAllocator, this);
}

AKRESULT DBoxSinkVista::IsDataNeeded( AkUInt16 & out_uBuffersNeeded )
{
	UINT32 uPaddingFrames;
	
	if ( m_spClientOut == NULL )
		return AK_Fail;
	
	HRESULT hr = m_spClientOut->GetCurrentPadding( &uPaddingFrames );
	if ( hr == AK_Fail ) 
		return AK_Fail;

	m_bStarved = uPaddingFrames == 0;

	out_uBuffersNeeded = (AkUInt16)(( m_uBufferFrames - uPaddingFrames ) / (DBOX_NUM_REFILL_FRAMES * SAMPLE_MULTIPLIER));

	ManageDrift(m_uBufferFrames - uPaddingFrames);

	return AK_Success;
}

AKRESULT DBoxSinkVista::PassData(AkPipelineBuffer& io_rXfer)
{
	BYTE * pData = NULL;

	if ( m_spRenderClient == NULL )
		return AK_Fail;

	if(io_rXfer.uValidFrames == 0)
		return AK_Success;

	//Always add 1 to take care of the possibility an extra frames because of the resampling factor (21.33333).
	//It will be corrected properly when we release the buffer.
	AkUInt16 lBufferFrames = (AkUInt16)(io_rXfer.MaxFrames() * SAMPLE_MULTIPLIER) + 1;
	HRESULT hr = m_spRenderClient->GetBuffer( lBufferFrames, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

	//Limit the signal within the device parameters.
	AkReal32* pSamples = (AkReal32*)io_rXfer.GetInterleavedData();
	m_oLimiter.Limit(pSamples, io_rXfer.MaxFrames(), m_Output1, m_Output2);

	AkUInt32 uOffset = 0;	//Always 0 on Vista.
	lBufferFrames = Fill<TraitVista>(pSamples, io_rXfer.MaxFrames(), (AkReal32*)pData, uOffset);

	hr = m_spRenderClient->ReleaseBuffer( lBufferFrames, 0 );

	return AK_Success;
}

AKRESULT DBoxSinkVista::PassSilence(AkUInt16 in_uNumFrames)
{
	if ( m_spRenderClient == NULL )
		return AK_Fail;

	//Note: no economic mode on Vista because we need to keep the buffer active to keep the device active.
	
	BYTE * pData = NULL;

	//Always add 1 to take care of the possibility an extra frames because of the resampling factor (21.33333).
	//It will be corrected properly when we release the buffer.
	AkUInt16 lBufferFrames = (AkUInt16)(in_uNumFrames * SAMPLE_MULTIPLIER) + 1;
	HRESULT hr = m_spRenderClient->GetBuffer( lBufferFrames, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

	AkUInt32 uOffset = 0;	//Always 0 on Vista.
	lBufferFrames = FillSilence<TraitVista>((AkReal32*)pData, in_uNumFrames, uOffset);

	hr = m_spRenderClient->ReleaseBuffer( lBufferFrames, 0 );

	return AK_Success;
}

void DBoxSinkVista::Stop()
{
	m_spClientOut->Stop();
}
