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
// AkSink.cpp
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkSink.h"

#include <xmmintrin.h>

#if defined (_DEBUG)
#include "stdio.h"
#include "fcntl.h"
#include "io.h"

// PGG : hope that 256 will be enough
static AkTChar String[256];
#endif

#include "AkMath.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AkSettings.h"

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

#ifdef AKVISTA

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <propidl.h>
#include <propkey.h>

// REFERENCE_TIME time units per millisecond
#define MFTIMES_PER_MILLISEC  10000

#endif

#define AK_PRIMARY_BUFFER_FORMAT				WAVE_FORMAT_PCM
#define	AK_PRIMARY_BUFFER_BITS_PER_SAMPLE		(16)
#define	AK_PRIMARY_BUFFER_SAMPLE_TYPE			AkInt16

extern AkPlatformInitSettings g_PDSettings;

LPDIRECTSOUND8	CAkSinkDirectSound::m_pDirectSound = NULL;

static const AkUInt32 ulNumVoiceStartFrames = 2 * AK_NUM_VOICE_REFILL_FRAMES;


CAkSink::CAkSink()
{
	// device presence is unknown
	m_IsHardwareDevicePresent = false;

	// speakers config is unknown
	m_SpeakersConfig = 0;

#ifndef AK_OPTIMIZED
	m_stats.m_fOutMin = (AkReal32) INT_MAX;
	m_stats.m_fOutMax = (AkReal32) INT_MIN;
	m_stats.m_fOutSum = 0;
	m_stats.m_fOutSumOfSquares = 0;
	m_stats.m_uOutNum = 0;

	m_pCaptureStream = NULL;
	m_eCaptureState = NoCapture;
	AKPLATFORM::AkMemSet( m_CaptureFilename, 0, AKSINK_MAXCAPTURENAMELENGTH*sizeof(AkTChar) );
#endif
}

CAkSink * CAkSink::Create( AkPlatformInitSettings & in_settings, bool in_bDummy )
{
#ifdef AKVISTA
	// Try to create Vista sink
	IMMDeviceEnumerator * pEnumerator = NULL;
	CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator);
	if ( pEnumerator )
	{
		CAkSinkVista * pSinkVista = AkNew( g_LEngineDefaultPoolId, CAkSinkVista() );
		if ( !pSinkDSVista )
			return NULL;	// NOTE: this should NEVER happen

		if ( pSinkVista )
		{
			if ( pSinkVista->Init( pEnumerator ) != AK_Success )
			{
				pSinkVista->Term();
				AkDelete( g_LEngineDefaultPoolId, pSinkVista );
			}
			else
			{
				// Success
				pEnumerator->Release();
				return pSinkVista;
			}
		}
		pEnumerator->Release();
	}
#endif
	// Try to create DirectSound sink ( if we are not expressly asked for a dummy )

	if ( !in_bDummy )
	{
		CAkSinkDirectSound * pSinkDS = AkNew( g_LEngineDefaultPoolId, CAkSinkDirectSound() );

		if ( pSinkDS )
		{
			if ( pSinkDS->Init( in_settings.hWnd ) == AK_Success ) 
				return pSinkDS;

			pSinkDS->Term();
			AkDelete( g_LEngineDefaultPoolId, pSinkDS );
		}
	}

	// Create a dummy sink in the event that there is no sound card or driver 

	CAkSinkDummy * pSinkDummy = AkNew( g_LEngineDefaultPoolId, CAkSinkDummy() );
	if ( !pSinkDummy )
		return NULL;	// NOTE: this should NEVER happen
		
	pSinkDummy->Init();

	return pSinkDummy;
}

#ifndef AK_OPTIMIZED
void CAkSink::UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	__m128 * pmIn = (__m128 *) in_pfSamples;
	__m128 * pmEnd = (__m128 *) ( in_pfSamples + in_uNumSamples );

	__m128 mMin = _mm_set_ps1( (AkReal32) INT_MAX ); 
	__m128 mMax = _mm_set_ps1( (AkReal32) INT_MIN );
	__m128 mSum = _mm_set_ps1( 0.0f );
	__m128 mSumSquares = _mm_set_ps1( 0.0f );

	do
	{
		__m128 mTmp = *pmIn;

		mMin = _mm_min_ps( mTmp, mMin );
		mMax = _mm_max_ps( mTmp, mMax );
		mSum = _mm_add_ps( mTmp, mSum );

		mTmp = _mm_mul_ps( mTmp, mTmp );
		mSumSquares = _mm_add_ps( mTmp, mSumSquares );
		
		pmIn++;
	}
	while ( pmIn < pmEnd );

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin.m128_f32[ 0 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin.m128_f32[ 1 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin.m128_f32[ 2 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, mMin.m128_f32[ 3 ] );

	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax.m128_f32[ 0 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax.m128_f32[ 1 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax.m128_f32[ 2 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, mMax.m128_f32[ 3 ] );

	m_stats.m_fOutSum += mSum.m128_f32[0] + mSum.m128_f32[1] + mSum.m128_f32[2] + mSum.m128_f32[3];
	m_stats.m_fOutSumOfSquares += mSumSquares.m128_f32[0] + mSumSquares.m128_f32[1] + mSumSquares.m128_f32[2] + mSumSquares.m128_f32[3];

	m_stats.m_uOutNum += in_uNumSamples;
}

void CAkSink::UpdateProfileSilence( AkUInt32 in_uNumSamples )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, 0 );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, 0 );
	m_stats.m_uOutNum += in_uNumSamples;
}
#endif

// Prepare a buffer format for DirectSound.
void CAkSink::PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt )
{
	memset( &out_wfExt, 0, sizeof( WAVEFORMATEXTENSIBLE ) );

	AkUInt16 uNumChannels = (AkInt16)AK::GetNumChannels( m_SpeakersConfig );
	out_wfExt.Format.nChannels			= uNumChannels;
	out_wfExt.Format.nSamplesPerSec		= AK_CORE_SAMPLERATE;
	out_wfExt.Format.wBitsPerSample		= AK_PRIMARY_BUFFER_BITS_PER_SAMPLE;
	out_wfExt.Format.nBlockAlign		= sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE) * uNumChannels;
	out_wfExt.Format.nAvgBytesPerSec	= AK_CORE_SAMPLERATE * out_wfExt.Format.nBlockAlign;

	if ( m_SpeakersConfig == AK_SPEAKER_SETUP_STEREO  )
	{
		out_wfExt.Format.cbSize		= 0;
		out_wfExt.Format.wFormatTag	= AK_PRIMARY_BUFFER_FORMAT;
	}
	else
	{
		AKASSERT( m_SpeakersConfig == AK_SPEAKER_SETUP_5POINT1 );

		// Use WAVEFORMATEXTENSIBLE for 5.1, as Vista requires it.

		out_wfExt.Format.cbSize		= sizeof( WAVEFORMATEXTENSIBLE ) - sizeof( WAVEFORMATEX );
		out_wfExt.Format.wFormatTag	= WAVE_FORMAT_EXTENSIBLE;

		out_wfExt.dwChannelMask					= AK_SPEAKER_SETUP_5POINT1;
		out_wfExt.SubFormat						= KSDATAFORMAT_SUBTYPE_PCM;
		out_wfExt.Samples.wValidBitsPerSample	= out_wfExt.Format.wBitsPerSample;
	}
}

#ifndef AK_OPTIMIZED
//====================================================================================================
// FormatCaptureStreamHeader
// Format the header to be used by the output capture file
//====================================================================================================
void CAkSink::FormatCaptureStreamHeader(AkUInt32 in_uDataSize,
										AkWAVEFileHeader & out_CaptureSteamHeader)
{
	
	out_CaptureSteamHeader.RIFF.ChunkId = RIFFChunkId;
	out_CaptureSteamHeader.RIFF.dwChunkSize = sizeof(AkWAVEFileHeader) + in_uDataSize - 8;
	out_CaptureSteamHeader.uWAVE = WAVEChunkId;
	out_CaptureSteamHeader.fmt.ChunkId = fmtChunkId;
	out_CaptureSteamHeader.fmt.dwChunkSize = sizeof(WAVEFORMATEXTENSIBLE);
	WAVEFORMATEXTENSIBLE wfx;
	PrepareFormat( wfx );
	out_CaptureSteamHeader.fmtHeader = wfx; 
	out_CaptureSteamHeader.data.ChunkId = dataChunkId;
	out_CaptureSteamHeader.data.dwChunkSize = in_uDataSize;
}

//====================================================================================================
// WriteToCaptureStream
// Write out data to the capture stream
//====================================================================================================
bool CAkSink::WriteToCaptureStream(void *in_pData, AkUInt32 in_uDataSize)
{
	if(m_pCaptureStream)
	{
		AkUInt32 uOutSize;
		m_pCaptureStream->Write(in_pData, in_uDataSize,	true, 0, 0,	uOutSize);
		AKASSERT( uOutSize == in_uDataSize ); 
		m_uCaptureStreamDataSize += uOutSize;
		return true;
	}

	return false;
}

//====================================================================================================
// StartOutputCapture
//====================================================================================================
void CAkSink::StartOutputCapture(AkLpCtstr in_CaptureFileName)
{
	m_eCaptureState = StartCaptureFlagged;
	wcscpy( m_CaptureFilename, in_CaptureFileName );
}

//====================================================================================================
// DoStartOutputCapture
//====================================================================================================
AKRESULT CAkSink::DoStartOutputCapture()
{
	AKRESULT eResult = AK_Fail;

	// make sure it hasn't been opened yet
	if(!m_pCaptureStream)
	{
		// Try open the file in the language specific directory.
		AkFileSystemFlags fsFlags;
		fsFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
		fsFlags.uCodecID = AKCODECID_BANK;
		fsFlags.bIsLanguageSpecific = false;
		fsFlags.pCustomParam = NULL;
		fsFlags.uCustomParamSize = 0;

		eResult =  AK::IAkStreamMgr::Get()->CreateStd(	m_CaptureFilename, 
														&fsFlags,
														AK_OpenModeWriteOvrwr,
														m_pCaptureStream );

		if(eResult != AK_Success)
			goto Failed;

		// write dummy WAVE header block out 
		// we will be going back to re-write the block when sizes have been resolved
		AkUInt32 uOutSize;
		AkWAVEFileHeader CaptureSteamHeader;
		AKPLATFORM::AkMemSet( &CaptureSteamHeader, 0, sizeof(AkWAVEFileHeader) );
		eResult = m_pCaptureStream->Write(	(void *)&CaptureSteamHeader, 
											sizeof(AkWAVEFileHeader),
											true,
											0,
											0,
											uOutSize);		
		if ( eResult != AK_Success )
			goto Failed;

		m_uCaptureStreamDataSize = 0;
		m_eCaptureState = CaptureActive;
	}

	return eResult;

Failed:
	if ( m_pCaptureStream )
	{
		m_pCaptureStream->Destroy();
		m_pCaptureStream = NULL;
	}

	return eResult;
}

//====================================================================================================
// StopOutputCapture
//====================================================================================================
void CAkSink::StopOutputCapture()
{
	m_eCaptureState = StopCaptureFlagged;
}

//====================================================================================================
// DoStopOutputCapture
//====================================================================================================
AKRESULT CAkSink::DoStopOutputCapture()
{
	AKRESULT eResult = AK_Fail;

	// make sure the stream was previously opened
	if(m_pCaptureStream)
	{
		// Seek back to start of stream 
		eResult = m_pCaptureStream->SetPosition(0, AK_MoveBegin, NULL);
												
		if(eResult == AK_Success)
		{
			// format header block patch file sizes in header
			AkWAVEFileHeader CaptureSteamHeader;
			FormatCaptureStreamHeader( m_uCaptureStreamDataSize, CaptureSteamHeader);

			AkUInt32 uOutSize;
			eResult = m_pCaptureStream->Write(	(void *)&CaptureSteamHeader, 
												sizeof(AkWAVEFileHeader),
												true,
												0,
												0,
												uOutSize);
		}

		// Kill stream, whether or not we succeeded to write updated header
		m_pCaptureStream->Destroy();
		m_pCaptureStream = NULL;
		m_uCaptureStreamDataSize = 0;
		m_eCaptureState = NoCapture;
	}

	return eResult;
}
#endif


//====================================================================================================
//====================================================================================================
CAkSinkDirectSound::CAkSinkDirectSound()
{
	// no direct sound
	m_pDirectSound = NULL;

	// memory
	m_ulBufferSize = 0;
	m_pvStart = NULL;
	m_pvRefillPosition = NULL;
	m_lRefillToEnd = 0;
	m_ulRefillOffset = 0;
	m_uFreeRefillFrames = 0;
	m_uPlay = 0;
	m_uWrite = 0;

	// DS
	m_pDSBuffer = NULL;

	// flags
	m_bStarved = false;
}
//====================================================================================================
//====================================================================================================
CAkSinkDirectSound::~CAkSinkDirectSound()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::Init(HWND in_hWnd)
{
	DSBUFFERDESC	dsbd;
//----------------------------------------------------------------------------------------------------
// initialise DS
//----------------------------------------------------------------------------------------------------
	if (FAILED(CoCreateInstance(CLSID_DirectSound8,NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&m_pDirectSound)))
	{
		AKASSERT(!"Unable to create CLSID_DirectSound");
	}

	HRESULT result = m_pDirectSound->Initialize(NULL);

	if(FAILED(result))
	{
#if defined (_DEBUG)
		wsprintf(String, L"WARNING: Unable to initialize DirectSound");
		OutputDebugString(String);
#endif
		goto Failed;
	}

	if(FAILED(m_pDirectSound->SetCooperativeLevel(in_hWnd, DSSCL_PRIORITY)))
	{
		AKASSERT(!"Unable to set cooperative level");
		goto Failed;
	}
//----------------------------------------------------------------------------------------------------
// create the primary buffer
//----------------------------------------------------------------------------------------------------
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

	if(FAILED(m_pDirectSound->CreateSoundBuffer(&dsbd, &m_pPrimaryBuffer, NULL)))
	{
		AKASSERT(!"Unable to create the primary buffer");
		goto Failed;
	}
//----------------------------------------------------------------------------------------------------
// get DS capabilities
//----------------------------------------------------------------------------------------------------
	DSCAPS	Capabilities;		// Capabilities of the device

	Capabilities.dwSize = sizeof(DSCAPS);
	if (FAILED(m_pDirectSound->GetCaps(&Capabilities)))
	{
		AKASSERT(!"Unable to get Direct Sound capabilities");
		goto Failed;
	}

//----------------------------------------------------------------------------------------------------
// get speaker config
//----------------------------------------------------------------------------------------------------
	AkUInt32	dwSpeakerConfig;
	if(FAILED(m_pDirectSound->GetSpeakerConfig(&dwSpeakerConfig)))
	{
		AKASSERT(!"Unable to get speaker config");
		goto Failed;
	}
//----------------------------------------------------------------------------------------------------
// set the primary buffer's format
//----------------------------------------------------------------------------------------------------
	AKRESULT eResult = SetPrimaryBufferFormat(dwSpeakerConfig);
	if(eResult != AK_Success)
	{
		// fold back to stereo
		eResult = SetPrimaryBufferFormat(DSSPEAKER_STEREO);
		if(eResult != AK_Success)
		{
			return eResult;
		}
	}

	// SUCCESS!
	m_IsHardwareDevicePresent = true;

	return AK_Success;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
Failed:
	return AK_Fail;
}
//====================================================================================================
// get rid of all the leftovers before quiting
//====================================================================================================
void CAkSinkDirectSound::Term()
{
	if ( m_pDSBuffer )
	{
		// stop what's playing if needed
		m_pDSBuffer->Stop();

		// release the DS buffer
		m_pDSBuffer->Release();
		m_pDSBuffer = NULL;
	}

	if( m_pDirectSound )
	{
		m_pDirectSound->Release();
		m_pDirectSound = NULL;
	}

#ifndef AK_OPTIMIZED
	DoStopOutputCapture();
#endif
}
//====================================================================================================
// add a new one to the list of those we have
//====================================================================================================
AKRESULT CAkSinkDirectSound::Start( VoiceParameters& in_rParams )
{
	// temporary one
	IDirectSoundBuffer* pDSB = NULL;

	// Create a DSBuffer to play this sound
	DSBUFFERDESC desc;

	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);

	//TODO(alessard) check to not enable DSBCAPS if it is possible that it might not be used
	desc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
	if( g_PDSettings.bGlobalFocus )
	{
		desc.dwFlags |= DSBCAPS_GLOBALFOCUS;
	}

	WAVEFORMATEXTENSIBLE wfExt;
	desc.lpwfxFormat = (WAVEFORMATEX *) &wfExt;

	AkChannelMask uChannelMask = GetSpeakerConfig();
	if ( uChannelMask != AK_SPEAKER_SETUP_STEREO && uChannelMask != AK_SPEAKER_SETUP_5POINT1 )
	{
		AKASSERT(!"Unsupported speaker configuration");
		goto Failed;
	}
	
	AkUInt16 uChannels = (AkUInt16)AK::GetNumChannels( uChannelMask );
	desc.dwBufferBytes = in_rParams.uNumRefills * AK_NUM_VOICE_REFILL_FRAMES * uChannels * sizeof(AkInt16);
	m_usBlockAlign = uChannels*sizeof(AkInt16);
	
	PrepareFormat( wfExt );

	// this is the size of our buffer
	m_ulBufferSize = desc.dwBufferBytes;

	// all is free to be re-filled
	m_lRefillToEnd = m_ulBufferSize;
//-----------------------------------------------------------------------------
// create the DS secondary buffer
//-----------------------------------------------------------------------------
	HRESULT hr = m_pDirectSound->CreateSoundBuffer(&desc, &pDSB, NULL);
	// if the secondary buffer creation was fine then go on
	if(SUCCEEDED(hr))
	{
		void*		pData1 = NULL;
		AkUInt32		ulData1Bytes;
		void*		pData2 = NULL;
		AkUInt32		ulData2Bytes;

//-----------------------------------------------------------------------------
// get the interface to our secondary buffer
//-----------------------------------------------------------------------------
		hr = pDSB->QueryInterface(IID_IDirectSoundBuffer8, (void**)(&m_pDSBuffer));

		if(SUCCEEDED(hr))
		{
			// figure out where the buffer is in memory
			hr = m_pDSBuffer->Lock(0, m_ulBufferSize,
											&pData1, &ulData1Bytes,
											&pData2, &ulData2Bytes,
											DSBLOCK_FROMWRITECURSOR);
			if(SUCCEEDED(hr))
			{
				// save the starting address
				m_pvStart = pData1;
				m_pvRefillPosition = pData1;

				::ZeroMemory( m_pvStart, ulData1Bytes );

				hr = m_pDSBuffer->Unlock(pData1, ulData1Bytes, pData2, ulData2Bytes);

				if(SUCCEEDED(hr))
				{
					m_pDSBuffer->SetVolume(0);
					m_pDSBuffer->SetFrequency(AK_CORE_SAMPLERATE);
				}
				else
				{
					AKASSERT(!"Could not unlock buffer");
					goto Failed;
				}
			}
			else
			{
				AKASSERT(!"Could not lock buffer");
				goto Failed;
			}
		}
		else
		{
			AKASSERT(!"Could not get DS buffer");
			goto Failed;
		}

		pDSB->Release();
		pDSB = NULL;
	}
	else
	{
		AKASSERT(!"Could not create the secondary buffer");
		goto Failed;
	}

	return AK_Success;
//----------------------------------------------------------------------------------------------------
// when in big trouble this one cleans up everything before returning
//----------------------------------------------------------------------------------------------------
Failed:
#if defined (_DEBUG) || (_PROFILE)
	AssertHR(hr);
#endif

	if(m_pDSBuffer != NULL)
	{
		m_pDSBuffer->Release();
		m_pDSBuffer = NULL;
	}

	if(pDSB)
	{
		pDSB->Release();
	}

	return AK_Fail;
}
//====================================================================================================
//====================================================================================================
bool CAkSinkDirectSound::IsStarved()
{
	return m_bStarved;
}
//====================================================================================================
//====================================================================================================
void CAkSinkDirectSound::ResetStarved()
{
	m_bStarved = false;
}
//====================================================================================================
//=========================================================5===========================================
void CAkSinkDirectSound::Play()
{
	HRESULT hr = m_pDSBuffer->Play(0,0,DSBPLAY_LOOPING);
	AKASSERT( SUCCEEDED( hr ) );
}
//====================================================================================================
// set the primary buffer's format
// only the ones the mixer supports are allowed
//====================================================================================================
AKRESULT CAkSinkDirectSound::SetPrimaryBufferFormat(AkUInt32 in_ulSpeakerConfig)
{
	AKASSERT(m_pPrimaryBuffer != NULL);

	WAVEFORMATEXTENSIBLE wfExt;

	DSBUFFERDESC desc = {0,};

	switch(DSSPEAKER_CONFIG(in_ulSpeakerConfig))
	{
	case DSSPEAKER_QUAD:
	case DSSPEAKER_5POINT1:
	case DSSPEAKER_7POINT1:
	case DSSPEAKER_SURROUND:
// For some reason DSSPEAKER_5POINT1_SURROUND is not defined before version 10 whereas DSSPEAKER_7POINT1_SURROUND is.
#if (DIRECTSOUND_VERSION >= 0x1000)
	case DSSPEAKER_5POINT1_SURROUND:
	case DSSPEAKER_7POINT1_SURROUND:
#else
	case 0x00000008:
	case 0x00000009:
#endif
		m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
		
		desc.dwBufferBytes = g_PDSettings.uNumRefillsInVoice * AK_NUM_VOICE_REFILL_FRAMES * 6 * sizeof(AkInt16);
		break;
	
	case DSSPEAKER_STEREO:
	case DSSPEAKER_HEADPHONE:
	default: //Default, Use stereo.
		m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
		desc.dwBufferBytes = g_PDSettings.uNumRefillsInVoice * AK_NUM_VOICE_REFILL_FRAMES * 2 * sizeof(AkInt16);
		break;
	}

	while( 1 )
	{
		m_pPrimaryBuffer->Stop();

		PrepareFormat( wfExt );

		if(FAILED(m_pPrimaryBuffer->SetFormat( (WAVEFORMATEX *) &wfExt )))
		{
			AKASSERT(!"Unable to set Primary Buffer format");
			m_SpeakersConfig = 0;
			return AK_Fail;
		}
		if(FAILED(m_pPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING)))
		{
			AKASSERT(!"Unable to play Primary Buffer");
			m_SpeakersConfig = 0;
			return AK_Fail;
		}

		desc.dwSize = sizeof(desc);
		desc.dwFlags |= DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
		if( g_PDSettings.bGlobalFocus )
		{
			desc.dwFlags |= DSBCAPS_GLOBALFOCUS;
		}
		desc.lpwfxFormat = (WAVEFORMATEX *) &wfExt;

		IDirectSoundBuffer* pDSBuffer = NULL;

		if(FAILED(m_pDirectSound->CreateSoundBuffer(&desc, &pDSBuffer, NULL)))
		{
			//AKASSERT(!"Voice manager init failed, Could not create a secondary buffer");
			
			switch( m_SpeakersConfig )
			{
			case AK_SPEAKER_SETUP_5POINT1:
				m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
				desc.dwBufferBytes = g_PDSettings.uNumRefillsInVoice * AK_NUM_VOICE_REFILL_FRAMES * 2 * sizeof(AkInt16);
				break;
			default:
				AKASSERT(!"Voice manager init failed, Could not create a secondary buffer");
				m_SpeakersConfig = 0;
				return AK_CouldNotCreateSecBuffer;
			}
		}
		else
		{
			pDSBuffer->Release();
			return AK_Success;
		}
	}

	return AK_CouldNotCreateSecBuffer;
}
//====================================================================================================
// get buffer ready for being written to
//====================================================================================================
AKRESULT CAkSinkDirectSound::GetBuffer(BufferUpdateParams& io_Params)
{
	AKRESULT eResult = AK_Success;

	HRESULT hResult = m_pDSBuffer->Lock(io_Params.ulOffset,io_Params.ulBytes,
								&io_Params.pvAudioPtr1,&io_Params.ulAudioBytes1,
								&io_Params.pvAudioPtr2,&io_Params.ulAudioBytes2,
								io_Params.ulFlags);

	if(FAILED(hResult))
	{
		eResult = AK_Fail;
	}

	return eResult;
}
//====================================================================================================
// we are done writting the buffer
//====================================================================================================
AKRESULT CAkSinkDirectSound::ReleaseBuffer(BufferUpdateParams& in_rParams)
{
	AKRESULT eResult = AK_Success;

	HRESULT hResult = m_pDSBuffer->Unlock(in_rParams.pvAudioPtr1,
								in_rParams.ulAudioBytes1,
								in_rParams.pvAudioPtr2,
								in_rParams.ulAudioBytes2);
	if(FAILED(hResult))
	{
		eResult = AK_Fail;
	}

	return eResult;
}
//====================================================================================================
// figure out if anything needs to be re-filled
//====================================================================================================
AKRESULT CAkSinkDirectSound::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	AKRESULT eResult = GetRefillSize( m_uFreeRefillFrames );
	if ( eResult != AK_Success )
		return eResult;

	out_uBuffersNeeded = m_uFreeRefillFrames / AK_NUM_VOICE_REFILL_FRAMES;

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::PassData(AkPipelineBuffer& io_rXfer)
{
	if ( !m_pDSBuffer ) 
		return AK_Fail;

#ifndef AK_OPTIMIZED
	if ( m_eCaptureState == StopCaptureFlagged )
		DoStopOutputCapture();
	if ( m_eCaptureState == StartCaptureFlagged )
		DoStartOutputCapture();
#endif

	AKRESULT eResult = AK_Success;

	if(io_rXfer.uValidFrames != 0)
	{
		BufferUpdateParams	Params;

		memset(&Params,0,sizeof(Params));
		Params.ulFlags = DSBLOCK_ENTIREBUFFER;
		Params.ulBytes = m_ulBufferSize;

		// lock the buffer before trying to write there
		eResult = GetBuffer(Params);

		if(eResult == AK_Success)
		{
			// pass what we have

			DoRefill(io_rXfer);

			// try to release the buffer
			eResult = ReleaseBuffer(Params);
		}
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::PassSilence( AkUInt32 uNumFrames )
{
	if ( !m_pDSBuffer ) 
		return AK_Fail;

#ifndef AK_OPTIMIZED
	if ( m_eCaptureState == StopCaptureFlagged )
		DoStopOutputCapture();
	if ( m_eCaptureState == StartCaptureFlagged )
		DoStartOutputCapture( );
#endif

	AKRESULT eResult = AK_Success;

	if( uNumFrames != 0 )
	{
		BufferUpdateParams	Params;

		memset(&Params,0,sizeof(Params));
		Params.ulFlags = DSBLOCK_ENTIREBUFFER;
		Params.ulBytes = m_ulBufferSize;

		// lock the buffer before trying to write there
		eResult = GetBuffer(Params);

		if(eResult == AK_Success)
		{
			// pass what we have

			DoRefillSilence( uNumFrames );

			// try to release the buffer
			eResult = ReleaseBuffer(Params);
		}
	}

	return eResult;
}
//====================================================================================================
//====================================================================================================
static void FloatsToShorts( short * dst, float * src, int nFloats )
{
	// This routine peaks at about 2.1 gigabytes / second on a P4 3 GHz. Unrolling helps a bit on the P4,
	// makes a bigger difference on Athlon64.

	AKASSERT( !( nFloats % 16 ) ); // operates on 16 floats at a time

	__m128 * pmIn = (__m128 *) src;
	__m128 * pmEnd = (__m128 *) ( src + nFloats );
	__m64 * pmOut = (__m64 *) dst;

	__m128 mMul = _mm_set_ps1( AUDIOSAMPLE_SHORT_MAX ); // duplicate multiplier factor 4x
	while ( pmIn < pmEnd )
	{
		__m128 mTmp1 = _mm_mul_ps( pmIn[ 0 ], mMul );
		__m128 mTmp2 = _mm_mul_ps( pmIn[ 1 ], mMul );
		__m128 mTmp3 = _mm_mul_ps( pmIn[ 2 ], mMul );
		__m128 mTmp4 = _mm_mul_ps( pmIn[ 3 ], mMul );
		
		// manually in-lined version of _mm_cvtps_pi16, because inlining doesn't actually work !

		__m64 mShorts1 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp1), _mm_cvtps_pi32(_mm_movehl_ps(mTmp1, mTmp1)));
		_mm_stream_pi( pmOut, mShorts1 );

		__m64 mShorts2 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp2), _mm_cvtps_pi32(_mm_movehl_ps(mTmp2, mTmp2)));
		_mm_stream_pi( pmOut + 1, mShorts2 );

		__m64 mShorts3 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp3), _mm_cvtps_pi32(_mm_movehl_ps(mTmp3, mTmp3)));
		_mm_stream_pi( pmOut + 2, mShorts3 );

		__m64 mShorts4 = _mm_packs_pi32(_mm_cvtps_pi32(mTmp4), _mm_cvtps_pi32(_mm_movehl_ps(mTmp4, mTmp4)));
		_mm_stream_pi( pmOut + 3, mShorts4 );

		pmIn += 4;
		pmOut += 4;
	}
	_mm_empty();
}

void CAkSinkDirectSound::DoRefill(AkPipelineBuffer& in_AudioBuffer)
{
	AKASSERT( m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES );

	if(m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES)
	{
		AkInt32	lBufferSamples = in_AudioBuffer.uValidFrames * in_AudioBuffer.NumChannels();
		void*	pvFrom = in_AudioBuffer.GetInterleavedData();
		do
		{
			AkUInt32	ulToRefillSamples = min(lBufferSamples,(AkInt32)(m_lRefillToEnd / sizeof(AkInt16)));
			// this much will be refilled
			lBufferSamples -= ulToRefillSamples;

			FloatsToShorts( (short *) m_pvRefillPosition, (float *) pvFrom, ulToRefillSamples );

#ifndef AK_OPTIMIZED
			UpdateProfileData( (float *) pvFrom, ulToRefillSamples );
			WriteToCaptureStream(m_pvRefillPosition, ulToRefillSamples * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE));
#endif

			// move refill pointer
			m_pvRefillPosition = (AkInt16*) m_pvRefillPosition + ulToRefillSamples;
			// move source pointer
			pvFrom = (float *) pvFrom + ulToRefillSamples;

			// move refill offset
			m_ulRefillOffset = (AkUInt32)((AkChar*)m_pvRefillPosition - (AkChar*)m_pvStart);

			// over the limit ?
			if(m_ulRefillOffset >= m_ulBufferSize)
			{
				// get them back within the buffer's limits
				m_ulRefillOffset = 0;
				m_pvRefillPosition = m_pvStart;
			}
			// figure out the new refill left value
			if(m_ulRefillOffset >= m_uPlay)
			{
				m_lRefillToEnd = m_ulBufferSize - m_ulRefillOffset;
			}
			else
			{
				m_lRefillToEnd = m_uPlay - m_ulRefillOffset;
			}
		}
		while(lBufferSamples > 0);
	}
}

void CAkSinkDirectSound::DoRefillSilence( AkUInt32 in_uNumFrames )
{
	AKASSERT( m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES );

	if(m_uFreeRefillFrames >= AK_NUM_VOICE_REFILL_FRAMES)
	{
		AkInt32	lBufferBytes = in_uNumFrames * m_usBlockAlign;

#ifndef AK_OPTIMIZED
		UpdateProfileSilence( lBufferBytes / sizeof( AkReal32 ) );
#endif

		do
		{
			AkUInt32	ulToRefillBytes = min(lBufferBytes,m_lRefillToEnd);
			// this much will be refilled
			lBufferBytes -= ulToRefillBytes;

			::ZeroMemory( m_pvRefillPosition, ulToRefillBytes );

#ifndef AK_OPTIMIZED
			// write silence to capture file
			WriteToCaptureStream(m_pvRefillPosition, ulToRefillBytes);
#endif

			// move refill pointer
			m_pvRefillPosition = (AkUInt8*) m_pvRefillPosition + ulToRefillBytes;

			// move refill offset
			m_ulRefillOffset = (AkUInt32)((AkChar*)m_pvRefillPosition - (AkChar*)m_pvStart);

			// over the limit ?
			if(m_ulRefillOffset >= m_ulBufferSize)
			{
				// get them back within the buffer's limits
				m_ulRefillOffset = 0;
				m_pvRefillPosition = m_pvStart;
			}
			// figure out the new refill left value
			if(m_ulRefillOffset >= m_uPlay)
			{
				m_lRefillToEnd = m_ulBufferSize - m_ulRefillOffset;
			}
			else
			{
				m_lRefillToEnd = m_uPlay - m_ulRefillOffset;
			}
		}
		while(lBufferBytes > 0);
	}
}

//====================================================================================================
//====================================================================================================
AKRESULT CAkSinkDirectSound::GetRefillSize( AkUInt32& out_uRefillFrames )
{
	// figure out where the play position is

	HRESULT hr = m_pDSBuffer->GetCurrentPosition(&m_uPlay, &m_uWrite);
	if ( hr != DS_OK )
		return AK_Fail;

	// handle cases where play and write are 0
	if((m_uPlay + m_uWrite) == 0)
	{
		// return a number of frames
		out_uRefillFrames = m_ulBufferSize / m_usBlockAlign;
		return AK_Success;
	}

	AkUInt32 ulFreeRefillSize = 0;
	m_lRefillToEnd = m_ulBufferSize - m_ulRefillOffset;
//----------------------------------------------------------------------------------------------------
//          P        W       R
// +--------+--------+-------+--------+
// |        |xxxxxxxx|       |        |
// +--------+--------+-------+--------+
//----------------------------------------------------------------------------------------------------
	if((m_ulRefillOffset > m_uPlay)
		&& (m_ulRefillOffset >= m_uWrite))
	{
		ulFreeRefillSize = m_ulBufferSize - m_ulRefillOffset + m_uPlay;
	}
//----------------------------------------------------------------------------------------------------
//    W    R                   P
// +--+----+-------------------+------+
// |xx|    |                   |xxxxxx|
// +--+----+-------------------+------+
//----------------------------------------------------------------------------------------------------
	else if((m_ulRefillOffset >= m_uWrite)
		&& (m_ulRefillOffset < m_uPlay))
	{
		ulFreeRefillSize = m_uPlay - m_ulRefillOffset;
	}
//----------------------------------------------------------------------------------------------------
//     R    P        W
// +---+----+--------+----------------+
// |   |    |xxxxxxxx|                |
// +---+----+--------+----------------+
//----------------------------------------------------------------------------------------------------
	else if((m_ulRefillOffset <= m_uPlay)
		&& (m_ulRefillOffset < m_uWrite))
	{
		ulFreeRefillSize = m_uPlay - m_ulRefillOffset;
	}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	else
	{
		m_bStarved = true;
	}

	// return a number of frames
	out_uRefillFrames = ulFreeRefillSize / m_usBlockAlign;

	return AK_Success;
}

#if defined (_DEBUG) || (_PROFILE)
void CAkSinkDirectSound::AssertHR(HRESULT hr)
{
	switch(hr)
	{
	case DSERR_ALLOCATED:
	AKASSERT(!"DSERR_ALLOCATED");
		break;
	case DSERR_CONTROLUNAVAIL:
	AKASSERT(!"DSERR_CONTROLUNAVAIL");
		break;
	case DSERR_INVALIDPARAM:
	AKASSERT(!"DSERR_INVALIDPARAM");
		break;
	case DSERR_INVALIDCALL:
	AKASSERT(!"DSERR_INVALIDCALL");
		break;
	case DSERR_GENERIC:
	AKASSERT(!"DSERR_GENERIC");
		break;
	case DSERR_PRIOLEVELNEEDED:
	AKASSERT(!"DSERR_PRIOLEVELNEEDED");
		break;
	case DSERR_OUTOFMEMORY:
	AKASSERT(!"DSERR_OUTOFMEMORY");
		break;
	case DSERR_BADFORMAT:
	AKASSERT(!"DSERR_BADFORMAT");
		break;
	case DSERR_UNSUPPORTED:
	AKASSERT(!"DSERR_UNSUPPORTED");
		break;
	case DSERR_NODRIVER:
	AKASSERT(!"DSERR_NODRIVER");
		break;
	case DSERR_ALREADYINITIALIZED:
	AKASSERT(!"DSERR_ALREADYINITIALIZED");
		break;
	case DSERR_NOAGGREGATION:
	AKASSERT(!"DSERR_NOAGGREGATION");
		break;
	case DSERR_BUFFERLOST:
	AKASSERT(!"DSERR_BUFFERLOST");
		break;
	case DSERR_OTHERAPPHASPRIO:
	AKASSERT(!"DSERR_OTHERAPPHASPRIO");
		break;
	case DSERR_UNINITIALIZED:
	AKASSERT(!"DSERR_UNINITIALIZED");
		break;
	case DSERR_NOINTERFACE:
	AKASSERT(!"DSERR_NOINTERFACE");
		break;
	case DSERR_ACCESSDENIED:
	AKASSERT(!"DSERR_ACCESSDENIED");
		break;
	case DSERR_BUFFERTOOSMALL:
	AKASSERT(!"DSERR_BUFFERTOOSMALL");
		break;
	default:
		AKASSERT(!"UNKNOWN HRESULT");
		break;
	}
}
#endif

// -----------------------------------------------------------------------------
#ifdef AKVISTA

CAkSinkVista::CAkSinkVista()
	: m_pDeviceOut( NULL )
	, m_pClientOut( NULL )
	, m_pRenderClient( NULL )
	, m_uBufferFrames( 0 )
#ifndef AK_OPTIMIZED
	, m_uNumChannels( 0 )
#endif
{
}

CAkSinkVista::~CAkSinkVista()
{
}

AKRESULT CAkSinkVista::Init( interface IMMDeviceEnumerator * in_pEnumerator )
{
	AKASSERT( in_pEnumerator );

	HRESULT hr;

	hr = in_pEnumerator->GetDefaultAudioEndpoint( eRender, eConsole, &m_pDeviceOut );
	if ( hr != S_OK )
		return AK_Fail;

    hr = m_pDeviceOut->Activate( __uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**) &m_pClientOut );
	if ( hr != S_OK )
		return AK_Fail;

	REFERENCE_TIME timeDefaultPeriod, timeMinPeriod;
	hr = m_pClientOut->GetDevicePeriod( &timeDefaultPeriod, &timeMinPeriod );

	// FIXME: apparently GetDevicePeriod gives values that are 'off' by a factor of 10
	timeMinPeriod *= 10;

	WAVEFORMATEXTENSIBLE wfExt;

	wfExt.dwChannelMask = AK_SPEAKER_SETUP_5POINT1;

	wfExt.Format.nChannels = 6;
	wfExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfExt.Format.nSamplesPerSec = AK_CORE_SAMPLERATE;
	wfExt.Format.nBlockAlign = wfExt.Format.nChannels * sizeof( AkReal32 );
	wfExt.Format.nAvgBytesPerSec = wfExt.Format.nSamplesPerSec * wfExt.Format.nBlockAlign;
	wfExt.Format.wBitsPerSample = sizeof( AkReal32 ) * 8;
	wfExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	wfExt.Samples.wValidBitsPerSample = 32;
	wfExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	WAVEFORMATEX * pWfSupported = NULL;
	hr = m_pClientOut->IsFormatSupported( AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX *) &wfExt, &pWfSupported );

	if ( pWfSupported ) 
	{
		// Format is not directly supported; we can handle changes in number of channels and
		// sample rate.

		AkAudioLibSettings::SetSampleFrequency( pWfSupported->nSamplesPerSec );
		wfExt.Format.nSamplesPerSec = pWfSupported->nSamplesPerSec;

		if ( pWfSupported->nChannels != 6 )
		{
			m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
		}
		else
		{
			m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
		}

		wfExt.Format.nChannels = AK::GetNumChannels( m_SpeakersConfig );

		wfExt.dwChannelMask = m_SpeakersConfig;
		wfExt.Format.nBlockAlign = pWfSupported->nChannels * sizeof( AkReal32 );
		wfExt.Format.nAvgBytesPerSec = wfExt.Format.nSamplesPerSec * pWfSupported->nBlockAlign;

		::CoTaskMemFree( pWfSupported );
		pWfSupported = NULL;
	}
	else // Format was accepted 'as-is'.
	{
		m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
	}

	// first start with the time for a refill.
	REFERENCE_TIME timeBuffer = (REFERENCE_TIME) MFTIMES_PER_MILLISEC * 1000 * AK_NUM_VOICE_REFILL_FRAMES / AK_CORE_SAMPLERATE;

	// now make sure we are going to do at least double-buffering of the system's buffer size,
	// rounded up to our refill size.
	timeBuffer = ( ( timeMinPeriod * 2 + timeBuffer - 1 ) / timeBuffer ) * timeBuffer;

	hr = m_pClientOut->Initialize( AUDCLNT_SHAREMODE_SHARED, 0, timeBuffer, 0, (WAVEFORMATEX *) &wfExt, NULL );

	if ( hr != S_OK )
		return AK_Fail;

	hr = m_pClientOut->GetBufferSize( &m_uBufferFrames );
	if ( hr != S_OK )
		return AK_Fail;

	AKASSERT( !( m_uBufferFrames % 1024 ) ); // result of our calculations should be a multiple of 1024 frames

#ifndef AK_OPTIMIZED
	m_uNumChannels = wfExt.Format.nChannels;
#endif

	// SUCCESS!
	m_IsHardwareDevicePresent = true;

	return AK_Success;
}

// CAkSink overrides

AKRESULT CAkSinkVista::Start(
	VoiceParameters& in_rParams
	)
{
	HRESULT hr = m_pClientOut->GetService( __uuidof(IAudioRenderClient), (void **) &m_pRenderClient );
	if ( hr != S_OK )
		return AK_Fail;

	return AK_Success;
}

void CAkSinkVista::Play()
{
	AKASSERT( m_pClientOut );
	m_pClientOut->Start();
}

void CAkSinkVista::Term()
{
	if ( m_pRenderClient )
	{
		m_pRenderClient->Release();
		m_pRenderClient = NULL;
	}

	if ( m_pClientOut )
	{
		m_pClientOut->Release();
		m_pClientOut = NULL;
	}

	if ( m_pDeviceOut )
	{
		m_pDeviceOut->Release();
		m_pDeviceOut = NULL;
	}
}

bool CAkSinkVista::IsStarved()
{
	return false; // FIXME: no starvation detection
}

void CAkSinkVista::ResetStarved()
{
}

AKRESULT CAkSinkVista::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	UINT32 uPaddingFrames;

	HRESULT hr = m_pClientOut->GetCurrentPadding( &uPaddingFrames );
	if ( hr == AK_Fail ) 
		return AK_Fail;

	out_uBuffersNeeded = ( m_uBufferFrames - uPaddingFrames ) / AK_NUM_VOICE_REFILL_FRAMES;

	return AK_Success;
}

static void ClipFloats( float * dst, float * src, int nFloats )
{
	AKASSERT( !( nFloats % 16 ) ); // operates on 16 floats at a time

	__m128 * pmIn = (__m128 *) src;
	__m128 * pmEnd = (__m128 *) ( src + nFloats );

	__m128 mMin = _mm_set_ps1( AUDIOSAMPLE_FLOAT_MIN ); // duplicate multiplier factor 4x
	__m128 mMax = _mm_set_ps1( AUDIOSAMPLE_FLOAT_MAX ); // duplicate multiplier factor 4x

	while ( pmIn < pmEnd )
	{
		__m128 mTmp1 = _mm_min_ps( pmIn[ 0 ], mMax );
		__m128 mTmp2 = _mm_min_ps( pmIn[ 1 ], mMax );
		__m128 mTmp3 = _mm_min_ps( pmIn[ 2 ], mMax );
		__m128 mTmp4 = _mm_min_ps( pmIn[ 3 ], mMax );

//      looks like we can get an unaligned output buffer from Vista, so we can't use these.
//		_mm_stream_ps( dst, _mm_max_ps( mTmp1, mMin ) );
//		_mm_stream_ps( dst + 4, _mm_max_ps( mTmp2, mMin ) );
//		_mm_stream_ps( dst + 8, _mm_max_ps( mTmp3, mMin ) );
//		_mm_stream_ps( dst + 12, _mm_max_ps( mTmp4, mMin ) );
		_mm_storeu_ps( dst, _mm_max_ps( mTmp1, mMin ) );
		_mm_storeu_ps( dst + 4, _mm_max_ps( mTmp2, mMin ) );
		_mm_storeu_ps( dst + 8, _mm_max_ps( mTmp3, mMin ) );
		_mm_storeu_ps( dst + 12, _mm_max_ps( mTmp4, mMin ) );

		pmIn += 4;
		dst += 16;
	}
}

AKRESULT CAkSinkVista::PassData(
	AkPipelineBuffer& io_rXfer
	)
{
	BYTE * pData = NULL;

	HRESULT hr = m_pRenderClient->GetBuffer( io_rXfer.uValidFrames, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	UpdateProfileData( (float *) pvFrom, io_rXfer.uValidFrames * io_rXfer.uNumChannels );
#endif

	ClipFloats( (float *) pData, (float *) io_rXfer.pucData, io_rXfer.uValidFrames * io_rXfer.uNumChannels );

#ifndef AK_OPTIMIZED
	WriteToCaptureStream(pData, io_rXfer.uValidFrames * io_rXfer.uNumChannels  * sizeof(float));
#endif

	hr = m_pRenderClient->ReleaseBuffer( io_rXfer.uValidFrames, 0 );

	return AK_Success;
}

AKRESULT CAkSinkVista::PassSilence(
	AkUInt32 uNumFrames
	)
{
	BYTE * pData = NULL;

	HRESULT hr = m_pRenderClient->GetBuffer( uNumFrames, &pData );
	if ( hr != S_OK || pData == NULL )
		return AK_Fail;

#ifndef AK_OPTIMIZED
	UpdateProfileSilence( uNumFrames * m_uNumChannels );
	WriteToCaptureStream(pData, uNumFrames * m_uNumChannels * sizeof(float));
#endif

	hr = m_pRenderClient->ReleaseBuffer( uNumFrames, AUDCLNT_BUFFERFLAGS_SILENT );

	return AK_Success;
}

#endif // AKVISTA

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//
// In the event that there is no sound card or driver present, the application will revert to creating
// a dummy sink
//
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

AKRESULT CAkSinkDummy::Init()
{
	m_Timer = ::GetTickCount();
	m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
	m_dwMSPerBuffer = (DWORD) ( 1000.0 / ( (double) AK_CORE_SAMPLERATE / AK_NUM_VOICE_REFILL_FRAMES ) ); // duration of buffer in milliseconds 
	
	// NOTE: m_IsHardwareDevicePresent does not get set here

	return AK_Success;
}

AKRESULT CAkSinkDummy::Start(VoiceParameters& in_rParams)
{
	return AK_Success;
}

void CAkSinkDummy::Play()
{
	return;
}

void CAkSinkDummy::Term()
{
	return;
}

bool CAkSinkDummy::IsStarved()
{
	return false;
}

void CAkSinkDummy::ResetStarved()
{
	return;	
}

AKRESULT CAkSinkDummy::IsDataNeeded( AkUInt32 & out_uBuffersNeeded )
{
	DWORD tmp_Timer  = ::GetTickCount();
	
	if(tmp_Timer < m_Timer) // in case the timer wraps
	{
		out_uBuffersNeeded = 1;
	}
	else 
	{
		out_uBuffersNeeded = (tmp_Timer - m_Timer) / m_dwMSPerBuffer;
	}

	if ( out_uBuffersNeeded )
		m_Timer = tmp_Timer;

	return AK_Success;
}

AKRESULT CAkSinkDummy::PassData(AkPipelineBuffer& io_rXfer)
{
	return AK_Success;
}

AKRESULT CAkSinkDummy::PassSilence(AkUInt32 uNumFrames)
{
	return AK_Success;
}

