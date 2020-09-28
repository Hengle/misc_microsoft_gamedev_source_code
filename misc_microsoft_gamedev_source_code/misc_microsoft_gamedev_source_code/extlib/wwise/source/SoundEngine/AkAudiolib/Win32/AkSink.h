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

#include <dsound.h>

#include "AkParameters.h"
#include "AkCommon.h"
#include "AkWinAudioLib.h"
#include "PlatformAudiolibDefs.h" // FT
#include "AkLEngineStructs.h"
#include "AkFileParserBase.h"
#include <ks.h>
#include <ksmedia.h>

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#define AKSINK_MAXCAPTURENAMELENGTH				(128)

// PhM : specifying the voice buffer is not supported in the PC version
struct VoiceParameters
{
	AkUInt32 uNumRefills;		// number of refills in voice buffer
};

enum AkCaptureStreamState
{
	NoCapture = 0,
	StartCaptureFlagged,
	CaptureActive,
	StopCaptureFlagged,
};

struct BufferUpdateParams
{
	AkUInt32	ulOffset;
	AkUInt32	ulBytes;
	void*		pvAudioPtr1;
	AkUInt32	ulAudioBytes1;
	void*		pvAudioPtr2;
	AkUInt32	ulAudioBytes2;
	AkUInt32	ulFlags;
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

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

class CAkSink
	: public CAkObject
{
public:
	static CAkSink * Create( AkPlatformInitSettings & in_settings, bool in_bDummy = false );

	virtual AKRESULT Start(
		VoiceParameters& in_rParams
		) = 0;

	virtual void Play() = 0;
	virtual void Term() = 0;

	AkForceInline bool IsHardwareDevicePresent()   { return m_IsHardwareDevicePresent; }

	AkForceInline AkChannelMask GetSpeakerConfig() { return m_SpeakersConfig; }
	void PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt );

	virtual bool IsStarved() = 0;
	virtual void ResetStarved() = 0;

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded ) = 0;

	virtual AKRESULT PassData(
		AkPipelineBuffer& io_rXfer
		) = 0;

	virtual AKRESULT PassSilence(
		AkUInt32 uNumFrames
		) = 0;

	virtual IDirectSound8 * GetDirectSoundInstance() = 0; // FIXME: this won't work in the long term...

#ifndef AK_OPTIMIZED
	void StartOutputCapture(AkLpCtstr in_CaptureFileName);
	void StopOutputCapture();
	AkSinkStats m_stats;
#endif

protected:
	CAkSink();

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

	AkChannelMask m_SpeakersConfig;				// speakers config
	bool m_IsHardwareDevicePresent;				// true if hardware is available
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

class CAkSinkDirectSound : public CAkSink
{
public:
	CAkSinkDirectSound();
	~CAkSinkDirectSound();

	AKRESULT Init(
		HWND in_hWnd
		);

	void Term();
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	// starts a voice according to what's in Params if possible
	AKRESULT Start(
		VoiceParameters& in_rParams
		);
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	void Play();
	bool IsStarved();
	void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	AKRESULT PassData(
		AkPipelineBuffer& io_rXfer
		);

	AKRESULT PassSilence(
		AkUInt32 uNumFrames
		);
	
//====================================================================================================
//====================================================================================================
	IDirectSound8 * GetDirectSoundInstance() { return m_pDirectSound; }
protected:
#if defined (_DEBUG) || (_PROFILE)
	void AssertHR(HRESULT hr);
#endif
//====================================================================================================
//====================================================================================================
private:
//----------------------------------------------------------------------------------------------------
// direct sound
//----------------------------------------------------------------------------------------------------
	AKRESULT SetPrimaryBufferFormat(
		AkUInt32 in_ulSpeakerConfig
		);
//----------------------------------------------------------------------------------------------------
// data transfer
//----------------------------------------------------------------------------------------------------
	AKRESULT GetBuffer(
		BufferUpdateParams&	io_rParams
		);

	AKRESULT ReleaseBuffer(
		BufferUpdateParams&	in_rParams
		);

	void DoRefill(
		AkPipelineBuffer&	in_AudioBuffer
		);

	void DoRefillSilence(
		AkUInt32	in_uNumFrames 
		);

	AKRESULT GetRefillSize(
		AkUInt32&   out_uRefillFrames
		);

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	// DS entry point
	static LPDIRECTSOUND8	m_pDirectSound;			// Pointer to the DirectSound Object
//----------------------------------------------------------------------------------------------------
// DS primary buffer things
//----------------------------------------------------------------------------------------------------
	LPDIRECTSOUNDBUFFER		m_pPrimaryBuffer;			// pointer on the primary buffer
	LPDIRECTSOUNDBUFFER8	m_pDSBuffer;				// Corresponding Direct Sound Buffer

	AkUInt32				m_ulBufferSize;				// playing buffer size
	void*					m_pvStart;					// buffer's memory address

	void*					m_pvRefillPosition;			// current refill position
	AkUInt32				m_ulRefillOffset;			// current offset from start
	AkInt32					m_lRefillToEnd;				// what's refill-able
	AkUInt32				m_uFreeRefillFrames;		// What's free when IsDataNeeded() is called
	AkUInt32				m_uPlay;					// play position when IsDataNeeded() is called
	AkUInt32				m_uWrite;					// write position when IsDataNeeded() is called
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	AkUInt16				m_usBlockAlign;

	// status
	bool					m_bStarved; 
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

#ifdef AKVISTA

class CAkSinkVista 
	: public CAkSink
{
public:
	CAkSinkVista();
    ~CAkSinkVista();

	AKRESULT Init( interface IMMDeviceEnumerator * in_pEnumerator );

	// CAkSink overrides

	virtual AKRESULT Start(
		VoiceParameters& in_rParams
		);

	virtual void Play();
	virtual void Term();

	virtual bool IsStarved();
	virtual void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData(
		AkPipelineBuffer& io_rXfer
		);

	virtual AKRESULT PassSilence(
		AkUInt32 uNumFrames
		);

	virtual IDirectSound8 * GetDirectSoundInstance() { return NULL; }

private:
    interface IMMDevice * m_pDeviceOut;
	interface IAudioClient * m_pClientOut;
	interface IAudioRenderClient * m_pRenderClient;

	AkUInt32 m_uBufferFrames;	// Number of audio frames in output buffer.
#ifndef AK_OPTIMIZED
	AkUInt32 m_uNumChannels;
#endif
};

#endif

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
//
// In the event that there is no sound card or driver present, the application will revert to creating
// a dummy sink
//
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
class CAkSinkDummy : public CAkSink
{
	DWORD m_Timer;
	DWORD m_dwMSPerBuffer;

public:
	CAkSinkDummy(){};
	~CAkSinkDummy(){};

	// Used by Create 
	AKRESULT Init();	
	
	// CAkSink overrides
	virtual AKRESULT Start(VoiceParameters& in_rParams);

	virtual void Play();
	virtual void Term();

	virtual bool IsStarved();
	virtual void ResetStarved();

	virtual AKRESULT IsDataNeeded( AkUInt32 & out_uBuffersNeeded );

	virtual AKRESULT PassData(AkPipelineBuffer& io_rXfer);

	virtual AKRESULT PassSilence(AkUInt32 uNumFrames);

	virtual IDirectSound8 * GetDirectSoundInstance() { return NULL; }
};

//----------------------------------------------------------------------------------------------------

extern CAkSink* g_pAkSink;
