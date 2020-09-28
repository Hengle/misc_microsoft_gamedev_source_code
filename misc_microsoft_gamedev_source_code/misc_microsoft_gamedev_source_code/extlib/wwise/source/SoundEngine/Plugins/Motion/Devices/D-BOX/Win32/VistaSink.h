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

#pragma once

#include "SinkBase.h"
#include <AkSmartPtr.h>

class DBoxSinkVista : public CSinkBase
{
public:
	DBoxSinkVista();
	~DBoxSinkVista();

	AKRESULT Init(AkUInt32 in_iBufferFrames);
	void	 Term(AK::IAkPluginMemAlloc * in_pAllocator);

	AKRESULT IsDataNeeded(AkUInt16 & out_uBuffersNeeded);
	AKRESULT PassData(AkPipelineBuffer& io_rXfer);
	AKRESULT PassSilence(AkUInt16 in_uNumFrames);
	void	 Stop();

private:
	static void PrepareFormat( WAVEFORMATEXTENSIBLE & out_wfExt );

	CAkSmartPtr<interface IMMDevice>			m_spDeviceOut;
	CAkSmartPtr<interface IAudioClient>			m_spClientOut;
	CAkSmartPtr<interface IAudioRenderClient>	m_spRenderClient;

	AkUInt32				m_uBufferFrames;			//Number of audio frames in output buffer.
};
