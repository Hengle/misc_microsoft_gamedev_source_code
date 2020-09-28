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
// AkSink.h
//
// Platform dependent part
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xaudio.h"
#include "xauddefs.h"
#include "Audiodefs.h" // WAVEFORMATEXTENSIBLE

#include <AK/Tools/Common/AkLock.h>
#include "AkParameters.h"
#include "PlatformAudiolibDefs.h" // FT
#include "AkLEngineStructs.h"
#include "AkFileParserBase.h"

#define AK_MAX_VOICE_OUTPUTS	(1)		// mastering
#define AK_SUBMITPACKET_FLAGS	(0)

#define AK_MAX_PACKET_COUNT			(2)	
#define AK_NUM_VOICE_BUFFERS 		(2)

#define AKSINK_MAXCAPTURENAMELENGTH				(128)

//----------------------------------------------------------------------------------------------------
// things to help read the status
//----------------------------------------------------------------------------------------------------
#define SourceIsPlaying_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_STARTED) != 0)
#define SourceIsStopped_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_STARTED) == 0)
#define SourceIsStarting_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_STARTING) != 0)
#define SourceIsStopping_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_STOPPING) != 0)
#define SourceIsStarving_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_STARVED) != 0)
#define IsDataNeeded_M(_xsvs)			(((_xsvs) & XAUDIOSOURCESTATE_READYPACKET) != 0)
#define SourceIsSynched_M(_xsvs)		(((_xsvs) & XAUDIOSOURCESTATE_SYNCHRONIZED) != 0)

struct VoiceParameters
{
	AkUInt32			uNumRefills;		// number of refills in voice buffer
	XAUDIOSOURCEFORMAT*	pAudioFormat;		// format info
	HANDLE              hEventPacketDone;   // Handle to event to set when packet done
};

enum AkCaptureStreamState
{
	NoCapture = 0,
	StartCaptureFlagged,
	CaptureActive,
	StopCaptureFlagged,
};

struct AkWAVEFileHeader
{
	ChunkHeader		RIFF;			// AkFourcc	ChunkId: FOURCC('RIFF')
									// AkUInt32	dwChunkSize: Size of file (in bytes) - 8
	AkFourcc		uWAVE;			// FOURCC('WAVE')
	ChunkHeader		fmt;			// AkFourcc	ChunkId: FOURCC('fmt ')
									// AkUInt32	dwChunkSize: Total size (in bytes) of fmt chunk's content
	WAVEFORMATEXTENSIBLE fmtHeader;	// AkUInt16	wFormatTag: WAVEFORMATEXTENSIBLE
									// AkUInt16	nChannels: Number of channels (1=mono, 2=stereo etc.)
									// AkUInt32	nSamplesPerSec: Sample rate (e.g. 44100)
									// AkUInt32	nAvgBytesPerSec: nSamplesPerSec * nBlockAlign
									// AkUInt16 nBlockAlign: nChannels * nBitsPerSample / 8 for PCM
									// AkUInt16 wBitsPerSample: 8, 16, 24 or 32
									// AkUInt16 cbSize:	Size of WAVEFORMATEXTENSIBLE additional information
	ChunkHeader		data;			// AkFourcc	ChunkId: FOURCC('data')
									// AkUInt32	dwChunkSize: Total size (in bytes) of data chunk's content
	// Sample data goes after this..
};

class CAkSink : public CAkObject
{
public:
	CAkSink();
	~CAkSink();

	static CAkSink * Create( AkPlatformInitSettings & in_settings );

	AKRESULT Init();

	void Term();
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	// starts a voice according to what's in Params if possible
	AKRESULT Start(
		VoiceParameters& in_rParams
		);
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	AkForceInline AkChannelMask GetSpeakerConfig() { return m_SpeakersConfig; }
	void PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt );
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	void Play();
	void Stop();
	bool IsStarved();
	void ResetStarved();

	bool IsDataReadyRB();

	AkUInt16 IsDataNeededRB();

	bool IsDataNeededVoice();

	AKRESULT SubmitPacketRB();

	AKRESULT PassData(
		AkAudioBuffer& io_rXfer
		);

	AKRESULT PassSilence(
		AkUInt32 uNumFrames
		);

	CAkLock m_RBLock;

#ifndef AK_OPTIMIZED
	void StartOutputCapture(AkLpCtstr in_CaptureFileName);
	void StopOutputCapture();
	AkSinkStats m_stats;
#endif

//====================================================================================================
//====================================================================================================
private:
	void SetChannelMapEntries(
		XAUDIOCHANNELMAPENTRY* in_pChannelMapEntries,
		AkUInt32					in_ulNumChannels
		);
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	static void PacketDone(
		LPCXAUDIOVOICEPACKETCALLBACK pCallbackData
		);
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#ifndef AK_OPTIMIZED
	void UpdateProfileData( AkReal32 * in_pfSamples, AkUInt32 in_uNumSamples );
	void UpdateProfileSilence( AkUInt32 in_uNumSamples );
	AKRESULT DoStartOutputCapture();
	AKRESULT DoStopOutputCapture();
	void FormatCaptureStreamHeader(	AkUInt32 in_uDataSize,
									AkWAVEFileHeader & out_CaptureSteamHeader);
	bool WriteToCaptureStream(void *in_pData, AkUInt32 in_uDataSize);

	AkUInt32 m_uCaptureStreamDataSize;			// Data size capture counter (bytes)
	AK::IAkStdStream *m_pCaptureStream;			// Output stream for capture
	AkCaptureStreamState m_eCaptureState;
	AkTChar m_CaptureFilename[AKSINK_MAXCAPTURENAMELENGTH];
#endif

	AkUInt32				m_ulNumChannels;
	AkChannelMask			m_SpeakersConfig;

	// the XAudio source voice
	IXAudioSourceVoice*		m_pSourceVoice;

	// buffer things
    void *                m_pvAudioBuffer;
	AkUInt16				m_uWriteBufferIndex;	// Ring buffer write index		
	AkUInt16				m_uReadBufferIndex;		// Ring buffer read index
	void *					m_ppvRingBuffer[AK_NUM_VOICE_BUFFERS];	// Ring buffer
	AkUInt16				m_uNbBuffersRB;
	AkUInt16				m_usBlockAlign;

	// status
	bool					m_bStarved;					// 

	// Event to set when packet is done
	static HANDLE			m_hEventPacketDone;
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	// was capabilities
};

extern CAkSink* g_pAkSink;
