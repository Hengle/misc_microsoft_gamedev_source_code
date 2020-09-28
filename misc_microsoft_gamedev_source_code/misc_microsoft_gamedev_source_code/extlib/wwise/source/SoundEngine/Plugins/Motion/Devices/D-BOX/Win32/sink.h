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
// interface connected to the D-Box.
//////////////////////////////////////////////////////////////////////
#pragma once

#include <dsound.h>
#include <AkSmartPtr.h>
#include "SinkBase.h"


//Number of buffers.  6 Buffers gives a number of samples that is a power of 2.
#define BUFFER_COUNT 6

class DBoxSinkXP : public CSinkBase
{
public:
	DBoxSinkXP();
	~DBoxSinkXP();

	AKRESULT Init(HWND in_hwnd, AkUInt32 in_iRefillBuffers);
	void	 Term(AK::IAkPluginMemAlloc * in_pAllocator);

	AKRESULT PassData(AkPipelineBuffer& io_rXfer);
	AKRESULT PassSilence( AkUInt16 in_uNumFrames );
	AKRESULT IsDataNeeded( AkUInt16 & out_uBuffersNeeded );
	void	 Stop();

private:	
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

	AKRESULT SetPrimaryBufferFormat();
	AKRESULT Start();
	AKRESULT GetBuffer(BufferUpdateParams& io_Params);
	AKRESULT ReleaseBuffer(BufferUpdateParams& in_rParams);

	AKRESULT GetRefillSize( AkUInt32& out_uRefillFrames );

	CAkSmartPtr<IDirectSound8>		m_spDirectSound;
	CAkSmartPtr<IDirectSoundBuffer> m_spPrimaryBuffer;	// Pointer on the primary buffer
	CAkSmartPtr<IDirectSoundBuffer8> m_spDSBuffer;		// Corresponding Direct Sound Buffer

	AkInt8*					m_pvStart;					// Buffer start
	AkInt8*					m_pvEnd;					// Buffer end
	AkUInt32				m_ulRefillOffset;			// current offset from start
	AkUInt32				m_uFreeRefillFrames;		// What's free when IsDataNeeded() is called
	AkUInt32				m_uPlay;					// play position when IsDataNeeded() is called
	AkUInt32				m_uWrite;					// write position when IsDataNeeded() is called
	AkUInt32				m_ulBufferSize;			
	AkUInt32				m_ulSilenceSamples;			//Count of silent samples

	enum eState{StatePlay, StateSilence, StateEconoSilence};
	eState					m_eState;					//Current playback state

	AkUInt32				m_ulBufferMask;

	static BOOL CALLBACK EnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);
	static void PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt );

	static GUID s_guidDBoxDevice;

};
