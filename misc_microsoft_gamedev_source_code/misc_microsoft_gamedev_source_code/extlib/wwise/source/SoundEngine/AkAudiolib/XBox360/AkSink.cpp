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
#include "process.h"

#include "AkMath.h"
#include "AkMonitor.h"
#include "AkSink.h"
#include "AudiolibDefs.h"
#include "AkSettings.h"

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

#include <vectorintrinsics.h>

extern AkMemPoolId g_LEngineDefaultPoolId;

HANDLE			CAkSink::m_hEventPacketDone = NULL;

static const __vector4 vkMin = { (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN, (AkReal32) INT_MIN };
static const __vector4 vkMax = { (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX, (AkReal32) INT_MAX };


#define AK_PRIMARY_BUFFER_FORMAT				0x0003 // normalized floating point samples
#define	AK_PRIMARY_BUFFER_BITS_PER_SAMPLE		(32)
#define	AK_PRIMARY_BUFFER_SAMPLE_TYPE			AkReal32

CAkSink::CAkSink()
{
	// speakers config is unknown
	m_SpeakersConfig = 0;

	// buffer
    m_pvAudioBuffer = NULL;
	m_usBlockAlign = 0;

	// no XAudio source voice
	m_pSourceVoice = NULL;

	// flags
	m_bStarved = false;

	m_uWriteBufferIndex = 0;
	m_uReadBufferIndex = 0;
	m_uNbBuffersRB = 0;
	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;

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
//====================================================================================================
//====================================================================================================
CAkSink::~CAkSink()
{
}
//====================================================================================================
//====================================================================================================
CAkSink * CAkSink::Create( AkPlatformInitSettings & in_settings )
{
	// Create sink

	CAkSink * pSink = AkNew( g_LEngineDefaultPoolId, CAkSink() );
	if ( !pSink )
		return NULL;

	if ( pSink->Init() != AK_Success ) 
	{
		pSink->Term();
		AkDelete( g_LEngineDefaultPoolId, pSink );
		return NULL;
	}

	// Success
	return pSink;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkSink::Init()
{
//----------------------------------------------------------------------------------------------------
// no capabilities on xenon
//----------------------------------------------------------------------------------------------------
	XAUDIOENGINEINIT XAudioInit;

	// max number of channels that a source or submix can have
	XAudioInit.MaxVoiceChannelCount = XAUDIOSPEAKER_COUNT;
	XAudioInit.SubmixStageCount = 1;
	XAudioInit.pMasteringVoiceInit = NULL;
	// PhM : SDK 2571
    XAudioInit.ThreadUsage = XAUDIO_THREAD;

    // no effects table
    XAudioInit.pEffectTable          = NULL;

	// if this one does not work we're screwed
	if(FAILED(XAudioInitialize(&XAudioInit)))
	{
		AKASSERT(!"Unable to initialise XAudio");
		goto Failed;
	}
//----------------------------------------------------------------------------------------------------
// get and set speaker config
//----------------------------------------------------------------------------------------------------
	AkUInt32 ulXAudioSpeakerConfig;
	if(XAudioGetSpeakerConfig(&ulXAudioSpeakerConfig) != S_OK)
	{
		AKASSERT(!"Unable to get speaker config");
		return AK_Fail;
	}

	// default value
	m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
	// convert to channels
	// Dolby Pro Logic II (5.0 matrix encoded mix).
	if(ulXAudioSpeakerConfig & XAUDIOSPEAKERCONFIG_ANALOG_SURROUND)
	{
		m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
	}
	// One channel, replicated to both outputs (if you have stereo outputs on your A/V pack). 
	else if(ulXAudioSpeakerConfig & XAUDIOSPEAKERCONFIG_ANALOG_MONO)
	{
		m_SpeakersConfig = AK_SPEAKER_SETUP_STEREO;
	}
	// Dolby Digital (discrete 5.1 encoded). Requires A/V pack with digital (optical or coax) output.
	else if(ulXAudioSpeakerConfig & XAUDIOSPEAKERCONFIG_DIGITAL_DOLBYDIGITAL)
	{
		m_SpeakersConfig = AK_SPEAKER_SETUP_5POINT1;
	}
	else
	{
		AKASSERT(!"Invalid speaker configuration");
	}

	return AK_Success;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
Failed:
	return AK_Fail;
}
//====================================================================================================
// get rid of all the leftovers before quiting
//====================================================================================================
void CAkSink::Term()
{
	// anyone got lost in here ?
	Stop();

	if ( m_pSourceVoice ) 
		m_pSourceVoice->Release();

	// stop the XAudio system
	XAudioShutDown();

	if ( m_pvAudioBuffer )
		AkFree( g_LEngineDefaultPoolId, m_pvAudioBuffer );
	m_pvAudioBuffer = NULL;

#ifndef AK_OPTIMIZED
	DoStopOutputCapture();
#endif
}
//====================================================================================================
// add a new one to the list of those we have
//====================================================================================================
AKRESULT CAkSink::Start(VoiceParameters& in_rParams)
{
	XAUDIOSOURCEVOICEINIT	SourceVoiceInit;
	m_ulNumChannels = in_rParams.pAudioFormat->ChannelCount;
	AkUInt32 ulNumChannelMapEntries = m_ulNumChannels * XAUDIOSPEAKER_COUNT;

	m_hEventPacketDone = in_rParams.hEventPacketDone;

	// get things ready for outputs routing

	XAUDIOCHANNELMAPENTRY* pChannelMapEntries = (XAUDIOCHANNELMAPENTRY*)AkAlloc(g_LEngineDefaultPoolId,sizeof(XAUDIOCHANNELMAPENTRY) * ulNumChannelMapEntries);

	if(pChannelMapEntries == NULL)
	{
		return AK_InsufficientMemory;
	}

	AkChannelMask uChannelMask = GetSpeakerConfig();
	if ( uChannelMask != AK_SPEAKER_SETUP_STEREO && uChannelMask != AK_SPEAKER_SETUP_5POINT1 )
	{
		AKASSERT(!"Unsupported number of channels");
		goto Failed;
	}	
	AkUInt16 uChannels = (AkUInt16)AK::GetNumChannels( uChannelMask );
	AkUInt32 uBufferBytes = AK_NUM_VOICE_REFILL_FRAMES * uChannels * sizeof(AkReal32);
	m_usBlockAlign = uChannels*sizeof(AkReal32);

	// Allocate ring buffer 
    m_pvAudioBuffer = AkAlloc( g_LEngineDefaultPoolId, AK_NUM_VOICE_BUFFERS*uBufferBytes );
    if ( m_pvAudioBuffer == NULL )
    {
        goto Failed;
    }

	::ZeroMemory( m_pvAudioBuffer, AK_NUM_VOICE_BUFFERS*uBufferBytes );

	// Initialize ring buffer ptrs
	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = (AkUInt8*)m_pvAudioBuffer + i*uBufferBytes;

	// number of packets that can be queued
	SourceVoiceInit.MaxPacketCount = AK_MAX_PACKET_COUNT;

//----------------------------------------------------------------------------------------------------
// outputs routing
//----------------------------------------------------------------------------------------------------
	// set all volumes
	SetChannelMapEntries(pChannelMapEntries,m_ulNumChannels);

	XAUDIOVOICEOUTPUT		Output;
	XAUDIOVOICEOUTPUTENTRY	OutputEntry;
	XAUDIOCHANNELMAP		ChannelMap;

	// only outputing into the mastering voice
	Output.EntryCount = 1;
	Output.paEntries = &OutputEntry;

	// use the mastering voice
	OutputEntry.pDestVoice = NULL;
	// use this channel map
	OutputEntry.pChannelMap = &ChannelMap;

	// 
	ChannelMap.EntryCount = (XAUDIOCHANNEL)ulNumChannelMapEntries;
	ChannelMap.paEntries = pChannelMapEntries;
//----------------------------------------------------------------------------------------------------
// create the source voice
// XAudioCreateSourceVoice implies : <= 6 channels
//                                   8 or 16 bits pcm
//                                   1 < sampling frequency <= 65535
//----------------------------------------------------------------------------------------------------
	// copy the audio format
	SourceVoiceInit.Format = *in_rParams.pAudioFormat;
	// default category
	SourceVoiceInit.Category = XAUDIOVOICECATEGORY_NONE;
	// max outputs that this voice can have (mastering only)
	SourceVoiceInit.MaxOutputVoiceCount = AK_MAX_VOICE_OUTPUTS;
	// PhM : this should come from SpeakerConfig
	SourceVoiceInit.MaxChannelMapEntryCount = (XAUDIOCHANNEL)ulNumChannelMapEntries;
	// max up pitch octaves
	SourceVoiceInit.MaxPitchShift = 0;
	// pitch is now done outside of the voice
	SourceVoiceInit.Flags = XAUDIOSOURCEFLAGS_NOPITCH;

	// no hw effect
	SourceVoiceInit.pEffectChain = NULL;
	// set the output of this voice
	SourceVoiceInit.pVoiceOutput = &Output;
	// function called right before the audio frame is going to be processed
	SourceVoiceInit.pfnProcessCallback = NULL;
	// function called after the audio packet was processed
	SourceVoiceInit.pfnPacketCompletionCallback = &(CAkSink::PacketDone);
	// function called when a packet is played for the second time (PCM only)
	SourceVoiceInit.pfnPacketLoopCallback = NULL;
	// used to identifie the voice, passed to pfnProcessCallback & pfnPacketCallback <- ?
	SourceVoiceInit.pContext = (void*)this;

	// try to create the source voice
	if(SUCCEEDED(XAudioCreateSourceVoice( &SourceVoiceInit, &m_pSourceVoice)))
	{
		m_pSourceVoice->SetPitch(0.0f);
		m_pSourceVoice->SetVolume(1.0f);

		AkFree( g_LEngineDefaultPoolId, pChannelMapEntries );

		return AK_Success;
	}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
Failed:
	if ( pChannelMapEntries )
		AkFree( g_LEngineDefaultPoolId, pChannelMapEntries );

	return AK_Fail;
}
//====================================================================================================
// pChannelMapEntry -> Input channel 0, XAUDIOSPEAKER_FRONTLEFT, XAUDIOVOLUME_MAX
//                     Input channel 1, XAUDIOSPEAKER_FRONTLEFT, XAUDIOVOLUME_MAX
//                      ............
//                     Input channel 0, XAUDIOSPEAKER_FRONTRIGHT, XAUDIOVOLUME_MAX
//                     Input channel 1, XAUDIOSPEAKER_FRONTRIGHT, XAUDIOVOLUME_MAX
//                      ............
//====================================================================================================
void CAkSink::SetChannelMapEntries(XAUDIOCHANNELMAPENTRY*	in_pChannelMapEntries,
										AkUInt32					in_ulNumChannels)
{
	XAUDIOCHANNELMAPENTRY* pChannelMapEntry = in_pChannelMapEntries;

	for(AkUInt32 ulOutputChannel = 0; ulOutputChannel < XAUDIOSPEAKER_COUNT; ++ulOutputChannel)
	{
		for(AkUInt32 ulInputChannel = 0; ulInputChannel < m_ulNumChannels; ++ulInputChannel)
		{
			pChannelMapEntry->InputChannel = (XAUDIOCHANNEL)ulInputChannel;
			pChannelMapEntry->OutputChannel = (XAUDIOCHANNEL)ulOutputChannel;
			pChannelMapEntry->Volume = XAUDIOVOLUME_MIN;
			++pChannelMapEntry;
		}
	}

	// left -> left, right -> right, etc...
	pChannelMapEntry = in_pChannelMapEntries;
	for(AkUInt32 ulInputChannel = 0; ulInputChannel < m_ulNumChannels; ++ulInputChannel)
	{
		pChannelMapEntry->Volume = XAUDIOVOLUME_MAX;
		pChannelMapEntry += (m_ulNumChannels + 1);
	}
}
//====================================================================================================
//====================================================================================================
void CAkSink::Play()
{
	HRESULT hr = m_pSourceVoice->Start(0);
	AKASSERT( SUCCEEDED( hr ) );
}
//====================================================================================================
//====================================================================================================
void CAkSink::Stop()
{
	if( m_pSourceVoice )//must check since Stop may be called on Term after the Init Failed
	{
		// PhM : stop the voice
		m_pSourceVoice->Stop(0);
	}

	// Clear our ring buffer
	g_pAkSink->m_RBLock.Lock();
	m_uWriteBufferIndex = 0;
	m_uReadBufferIndex = 0;
	m_uNbBuffersRB = 0;
	for ( AkUInt32 i = 0; i < AK_NUM_VOICE_BUFFERS; ++i )
		m_ppvRingBuffer[i] = NULL;
	g_pAkSink->m_RBLock.Unlock();
}
//====================================================================================================
//====================================================================================================
bool CAkSink::IsStarved()
{
	return m_bStarved;
}
//====================================================================================================
//====================================================================================================
void CAkSink::ResetStarved()
{
	m_bStarved = false;
}
//====================================================================================================
// figure out if anything needs to be re-filled
//====================================================================================================
bool CAkSink::IsDataNeededVoice( )
{
	AKASSERT( m_pSourceVoice );

	XAUDIOSOURCESTATE State;
	m_pSourceVoice->GetVoiceState(&State);

	// set only, don't reset
	m_bStarved = m_bStarved || SourceIsStarving_M(State);

	if(IsDataNeeded_M(State) && !SourceIsStopping_M(State) && !SourceIsStopped_M(State))
	{
		return true;
	}
	return false;
}

bool CAkSink::IsDataReadyRB( )
{
	return ( m_uNbBuffersRB > 0 );
}

AkUInt16 CAkSink::IsDataNeededRB( )
{
	return ( AK_NUM_VOICE_BUFFERS - m_uNbBuffersRB );
}

AKRESULT CAkSink::SubmitPacketRB( )
{
	AKASSERT( m_pSourceVoice );

	AKRESULT eResult = AK_Success;
	// Build and submit the packet
	XAUDIOPACKET AudioPacket = { 0 };
	// TODO: Make sure buffers are always maximum size
	AudioPacket.BufferSize = AK_NUM_VOICE_REFILL_FRAMES * m_usBlockAlign;
	AudioPacket.pBuffer = m_ppvRingBuffer[m_uReadBufferIndex];
	++m_uReadBufferIndex;
	if ( m_uReadBufferIndex == AK_NUM_VOICE_BUFFERS )
		m_uReadBufferIndex = 0;
	AKASSERT( m_uNbBuffersRB > 0 );
	--m_uNbBuffersRB;
	HRESULT Hresult = m_pSourceVoice->SubmitPacket(&AudioPacket,AK_SUBMITPACKET_FLAGS);
	if(!SUCCEEDED(Hresult))
	{
		eResult = AK_Fail;
	}	
	return eResult;
}
//====================================================================================================
// this is the entry point for passing buffers
//====================================================================================================
AKRESULT CAkSink::PassData(AkAudioBuffer& io_rXfer)
{
	AKASSERT( m_pSourceVoice );

#ifndef AK_OPTIMIZED
	if ( m_eCaptureState == StopCaptureFlagged )
		DoStopOutputCapture();
	if ( m_eCaptureState == StartCaptureFlagged )
		DoStartOutputCapture();
#endif

	AKRESULT eResult = AK_Success;

	AkUInt32 uRefilledFrames = io_rXfer.uValidFrames;
	AKASSERT( uRefilledFrames != 0 );

	void* pvBuffer = m_ppvRingBuffer[m_uWriteBufferIndex];
	++m_uWriteBufferIndex;
	if ( m_uWriteBufferIndex == AK_NUM_VOICE_BUFFERS )
		m_uWriteBufferIndex = 0;
	++m_uNbBuffersRB;
	AKASSERT( m_uNbBuffersRB <= AK_NUM_VOICE_BUFFERS );
	AkUInt8* pbSourceData = (AkUInt8*)io_rXfer.GetInterleavedData();

	// copy the buffer
	if(pbSourceData != NULL )
	{
		AkInt32 lNumSamples = uRefilledFrames * m_ulNumChannels;
		AKASSERT( !( lNumSamples % 4 ) ); // we operate 4 samples at a time.

#ifndef AK_OPTIMIZED
		UpdateProfileData( (float *) pbSourceData, lNumSamples );
#endif
		// Clip samples to [-1.0, 1.0]

		__vector4 * AK_RESTRICT pvFrom = (__vector4 *) pbSourceData;
		__vector4 * AK_RESTRICT pvEnd = (__vector4 *) ( (AkReal32 *) pbSourceData + lNumSamples );
		__vector4 * AK_RESTRICT pvTo = (__vector4 *) pvBuffer;

		for ( ; pvFrom < pvEnd; pvFrom++, pvTo++ )
			*pvTo = __vmaxfp( vkMin, __vminfp( vkMax, *pvFrom ) );

#ifndef AK_OPTIMIZED
		// write to output file 
		WriteToCaptureStream(pvBuffer, lNumSamples * sizeof(AK_PRIMARY_BUFFER_SAMPLE_TYPE));
#endif
	}

	return eResult;
}

AKRESULT CAkSink::PassSilence( AkUInt32 uNumFrames )
{
	if ( !m_pSourceVoice ) 
		return AK_Fail;

#ifndef AK_OPTIMIZED
	if ( m_eCaptureState == StopCaptureFlagged )
		DoStopOutputCapture();
	if ( m_eCaptureState == StartCaptureFlagged )
		DoStartOutputCapture();
#endif

	AKASSERT( uNumFrames != 0 );

	void* pvBuffer = m_ppvRingBuffer[m_uWriteBufferIndex];
	++m_uWriteBufferIndex;
	if ( m_uWriteBufferIndex == AK_NUM_VOICE_BUFFERS )
		m_uWriteBufferIndex = 0;
	++m_uNbBuffersRB;
	AKASSERT( m_uNbBuffersRB <= AK_NUM_VOICE_BUFFERS );

#ifndef AK_OPTIMIZED
	UpdateProfileSilence( uNumFrames * m_usBlockAlign / sizeof( AkReal32 ) );
#endif
	
	int dataSize = uNumFrames * m_usBlockAlign;
	
	::ZeroMemory( pvBuffer, dataSize );

#ifndef AK_OPTIMIZED
	WriteToCaptureStream( pvBuffer, dataSize );
#endif	

	return AK_Success;
}

//====================================================================================================
//====================================================================================================

extern CAkSink * g_pAkSink;

void CAkSink::PacketDone(LPCXAUDIOVOICEPACKETCALLBACK pCallbackData)
{
	g_pAkSink->m_RBLock.Lock();

	bool bCanSubmit = g_pAkSink->IsDataNeededVoice() && g_pAkSink->IsDataReadyRB();
	if ( bCanSubmit )
		g_pAkSink->SubmitPacketRB();

	g_pAkSink->m_RBLock.Unlock();

	::SetEvent( m_hEventPacketDone );
}

#ifndef AK_OPTIMIZED
void CAkSink::UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	__vector4 * AK_RESTRICT pvFrom = (__vector4 *) in_pfSamples;
	__vector4 * AK_RESTRICT pvEnd = (__vector4 *) ( in_pfSamples + in_uNumSamples );

	__vector4 vecMin = vkMax;
	__vector4 vecMax = vkMin;
	__vector4 vecSum = __vspltisw( 0 );
	__vector4 vecSquares = __vspltisw( 0 );

	for ( ; pvFrom < pvEnd; pvFrom++ )
	{
		__vector4 vecTmp = *pvFrom;

		vecMin = __vminfp( vecTmp, vecMin );
		vecMax = __vmaxfp( vecTmp, vecMax );
		vecSum = __vaddfp( vecTmp, vecSum );

		vecSquares = __vmaddfp( vecTmp, vecTmp, vecSquares );
	}

	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, vecMin.v[ 0 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, vecMin.v[ 1 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, vecMin.v[ 2 ] );
	m_stats.m_fOutMin = AkMath::Min( m_stats.m_fOutMin, vecMin.v[ 3 ] );

	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, vecMax.v[ 0 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, vecMax.v[ 1 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, vecMax.v[ 2 ] );
	m_stats.m_fOutMax = AkMath::Max( m_stats.m_fOutMax, vecMax.v[ 3 ] );

	m_stats.m_fOutSum += vecSum.v[0] + vecSum.v[1] + vecSum.v[2] + vecSum.v[3];
	m_stats.m_fOutSumOfSquares += vecSquares.v[0] + vecSquares.v[1] + vecSquares.v[2] + vecSquares.v[3];

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

// Prepare a buffer format
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
		out_wfExt.Format.cbSize		= sizeof( WAVEFORMATEXTENSIBLE ) - sizeof( WAVEFORMATEX );
		out_wfExt.Format.wFormatTag	= WAVE_FORMAT_EXTENSIBLE;

		out_wfExt.dwChannelMask					= AK_SPEAKER_SETUP_5POINT1;
		out_wfExt.SubFormat						= KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
		out_wfExt.Samples.wValidBitsPerSample	= out_wfExt.Format.wBitsPerSample;
	}
}


void CAkSink::FormatCaptureStreamHeader(AkUInt32 in_uDataSize,
										AkWAVEFileHeader & out_CaptureSteamHeader)
{
	
	out_CaptureSteamHeader.RIFF.ChunkId = RIFXChunkId;
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
