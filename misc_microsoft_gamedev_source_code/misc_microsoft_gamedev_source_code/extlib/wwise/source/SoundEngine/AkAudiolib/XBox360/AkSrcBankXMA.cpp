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

#include "AkSrcBankXMA.h"

#include "AudiolibDefs.h"
#include "AkMonitor.h"
#include "xauddefs.h"
#include "XMADecoder.h"
#include <stdio.h>
#include "AkXMAHelpers.h"
#include "AkPlayingMgr.h"
#include "AkLEngine.h"

CAkSrcBankXMA::CAkSrcBankXMA( CAkPBI * in_pCtx )
	: CAkSrcBaseEx( in_pCtx )
	, m_pPlayback( NULL )
	, m_pXMADecodingBuffer( NULL )
	, m_pOutBuffer( NULL )
{
}

CAkSrcBankXMA::~CAkSrcBankXMA()
{
	ReleaseBuffer(); 
}

AKRESULT CAkSrcBankXMA::StartStream( )
{
    AkUInt8 * pBuffer;
    AkUInt32 uBufferSize;
    m_pCtx->GetDataPtr( pBuffer, uBufferSize );
    if ( pBuffer == NULL )
		return AK_Fail;

    XMA2WAVEFORMAT xmaFmt;
	AkXMA2CustomData xmaCustomData;

	bool bUpdateSampleOffset = true;
    AKRESULT eResult = InitXMA( pBuffer, uBufferSize, xmaFmt, xmaCustomData );
    if ( eResult == AK_Success )
    {
        HRESULT hr;

	    AkUInt16 uiLooping = m_pCtx->GetLooping();
	    if( IS_LOOPING( uiLooping ) )
	    {
            AkXMACustHWLoopData & loopData = xmaCustomData.loopData;
			
		    XMA_PLAYBACK_LOOP loop;

		    loop.dwLoopStartOffset = loopData.LoopStart;
		    loop.dwLoopEndOffset = loopData.LoopEnd;
		    loop.dwLoopSubframeEnd = loopData.SubframeData >> 4;
		    loop.dwLoopSubframeSkip = loopData.SubframeData & 0xf;
		    loop.numLoops = ( uiLooping == LOOPING_INFINITE ) ? 255 : min( 254, uiLooping-1 );

		    hr = XMAPlaybackSetLoop( m_pPlayback, 0, &loop );
		    AKASSERT( hr == S_OK );
	    }

		if( eResult == AK_Success )
		{
			AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
			if( l_numNativeSampleOffset )
			{
				AkUInt64 l_numRealSampleOffset = l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE;

				AkUInt32 uRealOffset = (AkUInt32)l_numRealSampleOffset;
				VirtualSeek( uRealOffset, xmaFmt, xmaCustomData.uSeekTableOffset, pBuffer );
				bUpdateSampleOffset = false;
				AKASSERT( l_numRealSampleOffset - uRealOffset >= 0 );
				m_pCtx->SetSourceOffset( (AkUInt32)(l_numRealSampleOffset) - uRealOffset );
			}
		}

	    hr = XMAPlaybackResumePlayback( m_pPlayback );
	    AKASSERT( hr == S_OK );
    }

	if( bUpdateSampleOffset )
	{
		m_uCurSampleOffset = 0;
	}
	return eResult;
}

void CAkSrcBankXMA::StopStream( )
{
	ReleaseBuffer();

	if ( m_pPlayback )
	{
		// Need to have the modify lock during PlaybackDestroy, otherwise the hardware
		// can still touch m_pXMADecodingBuffer after the call.
		HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
		if ( hr == S_OK )
			hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );

		AKVERIFY( XMAPlaybackDestroy( m_pPlayback ) == S_OK );
		m_pPlayback = NULL;
	}

	if( m_pXMADecodingBuffer )
	{
		AK::MemoryMgr::Falign( g_LEngineDefaultPoolId, m_pXMADecodingBuffer );
		m_pXMADecodingBuffer = NULL;
	}
	
	CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcBankXMA::StopLooping()
{
	XMA_PLAYBACK_LOOP loop;

	loop.dwLoopStartOffset = 0;
	loop.dwLoopEndOffset = 0;
	loop.dwLoopSubframeEnd = 0;
	loop.dwLoopSubframeSkip = 1;
	loop.numLoops = 0; // 0 == no looping

	HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
	AKASSERT( hr == S_OK );
	if ( hr != S_OK )
		return AK_Success;
	hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
	AKASSERT( hr == S_OK );
	if ( hr != S_OK ) 
		return AK_Success;

	hr = XMAPlaybackSetLoop( m_pPlayback, 0, &loop );
	AKASSERT( hr == S_OK );

	hr = XMAPlaybackResumePlayback( m_pPlayback );
	AKASSERT( hr == S_OK );

	return AK_Success;
}

void CAkSrcBankXMA::GetBuffer( AkVPLState & io_state )
{	
	HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
	AKASSERT( hr == S_OK );
	if ( hr != S_OK ) 
	{
		io_state.result = AK_Fail;
		return;
	}

	hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
	AKASSERT( hr == S_OK );
	if ( hr != S_OK ) 
	{
		io_state.result = AK_Fail;
		return;
	}

	DWORD dwAvail = XMAPlaybackQueryAvailableData( m_pPlayback, 0 );
	if ( dwAvail == 0 ) 
	{
		// No data available. Check if there was an error.
		if ( XMAPlaybackGetParseError( m_pPlayback, 0 ) != XMA_PLAYBACK_PARSE_ERROR_NONE 
			|| XMAPlaybackGetErrorBits( m_pPlayback, 0 ) != XMA_PLAYBACK_ERROR_NONE )
		{
			io_state.result = AK_Fail;
			return;
		}

		hr = XMAPlaybackResumePlayback( m_pPlayback );
		AKASSERT( hr == S_OK );

		io_state.result = AK_NoDataReady;
		return;
	}

	AkAudioFormat* pFormat = m_pCtx->GetMediaFormat();

	DWORD nReqSamples = io_state.buffer.MaxFrames();
	AKASSERT( nReqSamples >= XMADECODER_SUBFRAME_SIZE_IN_SAMPLES );

	AkUInt8 * pOutBuffer = NULL;

	DWORD dwConsumed = XMAPlaybackConsumeDecodedData( m_pPlayback, 0, nReqSamples, (void **) &pOutBuffer );

	// Allocate output buffer for decoded data (we don't want to hold XMA hardware decoder's output buffer too long
	AKASSERT( m_pOutBuffer == NULL);
	AkUInt32 uDecodedBufSize = dwConsumed * pFormat->GetNumChannels() * sizeof(AkInt16);
	m_pOutBuffer = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( LE_MAX_FRAMES_PER_BUFFER * pFormat->GetNumChannels() * sizeof(AkInt16) ); 
	if ( m_pOutBuffer == NULL )
	{
		io_state.result = AK_InsufficientMemory;
		hr = XMAPlaybackResumePlayback( m_pPlayback );
		AKASSERT( hr == S_OK );
        return;
	}
	AKPLATFORM::AkMemCpy( m_pOutBuffer, pOutBuffer, uDecodedBufSize );

	hr = XMAPlaybackResumePlayback( m_pPlayback );
	AKASSERT( hr == S_OK );

	AkUInt16 uNumFrames = (AkUInt16) ( dwConsumed );
	io_state.buffer.AttachInterleavedData( m_pOutBuffer, LE_MAX_FRAMES_PER_BUFFER, uNumFrames, pFormat->GetChannelMask() );

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = pFormat->uSampleRate;
		io_state.buffer.posInfo.uStartPos = m_uCurSampleOffset;
		io_state.buffer.posInfo.uFileEnd = m_uiTotalOutputSamples;
	}
	AkUInt32 uBackupCurSampleOffset = m_uCurSampleOffset;

    // IMPORTANT. Provide a temp variable to TimeSkip, in case there was an inconsistency between our sample count
    // and that of the decoder.
    // This can occur when XMAPlaybackSetDecodePosition() is called (VirtualOff with FromElapsedTime: WG-4939).
    AkUInt32 uLogicalConsumed = dwConsumed;
	bool bDidLoop = false;
	TimeSkipNoMarkers( uLogicalConsumed, bDidLoop ); //update our counters

	CopyRelevantMarkers( io_state.buffer, uBackupCurSampleOffset, bDidLoop );

	if ( dwConsumed == dwAvail ) 
	{
		// No more data available in output buffer -- we catched up with decoder. Could signify end of stream.
		if ( XMAPlaybackIsIdle( m_pPlayback, 0 ) ) 
		{
			io_state.result = AK_NoMoreData;
			return;
		}
	}

	io_state.result = AK_DataReady;
}

void CAkSrcBankXMA::ReleaseBuffer()
{
	if ( m_pOutBuffer )
	{
		CAkLEngine::ReleaseCachedAudioBuffer( LE_MAX_FRAMES_PER_BUFFER * m_pCtx->GetMediaFormat()->GetNumChannels() * sizeof(AkInt16), m_pOutBuffer );
		m_pOutBuffer = NULL;
	}
}

AkTimeMs CAkSrcBankXMA::GetDuration( void ) const
{
	// maths must be done in float to avoid overflow fix WG-1849
    AkUInt16 uNumLoop = m_pCtx->GetLooping();
    if ( uNumLoop != LOOPING_INFINITE )
        return (AkTimeMs) ( ( 1000.f * ( (AkReal32)(uNumLoop-1) * (AkReal32)( m_uPCMLoopEnd - m_uPCMLoopStart + 1 ) + (AkReal32)m_uiTotalOutputSamples ) ) / (AkReal32)m_pCtx->GetMediaFormat()->uSampleRate );
    else
        return 0;
}

AKRESULT CAkSrcBankXMA::TimeSkip( AkUInt32 & io_uFrames )
{	
	AkUInt32 uPosBeforeSkip = m_uCurSampleOffset;

	bool bDidLoop = false;
    AKRESULT eResult = TimeSkipNoMarkers( io_uFrames, bDidLoop );
    
	if( bDidLoop )
	{
		NotifyRelevantMarkers( uPosBeforeSkip, m_uPCMLoopEnd );
		NotifyRelevantMarkers( m_uPCMLoopStart, m_uPCMLoopStart + io_uFrames - ( m_uPCMLoopEnd - uPosBeforeSkip ) );
	}
	else
	{
		NotifyRelevantMarkers( uPosBeforeSkip, uPosBeforeSkip + io_uFrames );
	}

	UpdatePositionInfo( uPosBeforeSkip, m_uiTotalOutputSamples );

    return eResult;
}

AKRESULT CAkSrcBankXMA::TimeSkipNoMarkers( AkUInt32 & io_uFrames, bool& out_bDidLoop )
{
	AKRESULT eResult = AK_DataReady;

    m_uCurSampleOffset += io_uFrames;

    // Handle looping
	AkUInt32 ulEndLimit = DoLoop() ? m_uPCMLoopEnd : m_uiTotalOutputSamples;
    if ( m_uCurSampleOffset >= ulEndLimit )
    {
        if ( DoLoop() )
        {
            m_uCurSampleOffset -= (m_uPCMLoopEnd - m_uPCMLoopStart + 1);
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
			out_bDidLoop = true;
        }
        else if ( m_uLoopCnt == 1 )
        {
            io_uFrames -= (m_uCurSampleOffset - ulEndLimit);
            m_uCurSampleOffset = ulEndLimit;
            eResult = AK_NoMoreData;
        }
    }
    
    return eResult;
}

void CAkSrcBankXMA::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
    // "FromBeginning": flush data and set decoder position to the beginnign of data.
    // "FromElapsedTime": stop decoder, remember the current position in samples.

    if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
    {
        /* FIXME: RewindDecodePosition() does not work. Seems to be only useful when 
        decoder has finished decoding ALL buffer. So how do we "rewind"?
        Temp solution. Stop and restart Stream.

        // Rewind data to beginning of stream.
        HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
	    AKASSERT( hr == S_OK );

	    hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
	    AKASSERT( hr == S_OK );

        DWORD dwNumSamples = XMAPlaybackGetStreamPosition( m_pPlayback, 0 );

        AKVERIFY( XMAPlaybackRewindDecodePosition( m_pPlayback, 0, dwNumSamples ) );

        hr = XMAPlaybackResumePlayback( m_pPlayback );
	    AKASSERT( hr == S_OK );
        */
        
        StopStream();
    }
    else if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		// Need to have the modify lock during PlaybackDestroy, otherwise the hardware
		// can still touch m_pXMADecodingBuffer after the call.
		HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
		if ( hr == S_OK )
			hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
		
		AKVERIFY( XMAPlaybackDestroy( m_pPlayback ) == S_OK );
        m_pPlayback = NULL;
		if( m_pXMADecodingBuffer )
		{
			AK::MemoryMgr::Falign( g_LEngineDefaultPoolId, m_pXMADecodingBuffer );
			m_pXMADecodingBuffer = NULL;
		}
    }
}

AKRESULT CAkSrcBankXMA::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
    if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
    {
        // TEMP. Cannot rewind decoder, so stop and restart stream.
       return StartStream();
    }
    else if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
    {
        AKASSERT( m_pPlayback == NULL );

        // Reparse header, reinit XMA.
        AkUInt8 * pBuffer;
        AkUInt32 uBufferSize;
        m_pCtx->GetDataPtr( pBuffer, uBufferSize );
		AKASSERT( pBuffer );

		XMA2WAVEFORMAT xmaFmt;
		AkXMA2CustomData xmaCustomData;

		AKRESULT eResult = InitXMA( pBuffer, uBufferSize, xmaFmt, xmaCustomData );
		if ( eResult != AK_Success )
			return eResult;

		HRESULT hr;
	    if( DoLoop() )
	    {
            AkXMACustHWLoopData & loopData = xmaCustomData.loopData;

		    XMA_PLAYBACK_LOOP loop;

		    loop.dwLoopStartOffset = loopData.LoopStart;
		    loop.dwLoopEndOffset = loopData.LoopEnd;
		    loop.dwLoopSubframeEnd = loopData.SubframeData >> 4;
		    loop.dwLoopSubframeSkip = loopData.SubframeData & 0xf;
		    loop.numLoops = ( m_uLoopCnt == LOOPING_INFINITE ) ? 255 : min( 254, m_uLoopCnt-1 );

		    hr = XMAPlaybackSetLoop( m_pPlayback, 0, &loop );
		    AKASSERT( hr == S_OK );
	    }

        // Seek to appropriate place.

		VirtualSeek( m_uCurSampleOffset, xmaFmt, xmaCustomData.uSeekTableOffset, pBuffer );

	    hr = XMAPlaybackResumePlayback( m_pPlayback );
	    AKASSERT( hr == S_OK );
    }
	return AK_Success;
}

AKRESULT CAkSrcBankXMA::InitXMA(
    void * in_pBuffer, 
    AkUInt32 in_uBufferSize, 
    XMA2WAVEFORMAT & out_xmaFmt, 
	AkXMA2CustomData & out_xmaCustomData
	)
{
    AKRESULT eResult;

	// Do not parse markers if they were already parsed.
	CAkMarkers * pMarkers = ( m_markers.Count() == 0 ) ? &m_markers : NULL;

	eResult = CAkFileParser::Parse( in_pBuffer,      // Data buffer
                            in_uBufferSize,  // Buffer size
                            (WaveFormatEx *) &out_xmaFmt, // Returned audio format.
							sizeof( XMA2WAVEFORMAT ),	// NOTE: This will not work with more than 1 stream.
                            pMarkers,			// Markers.
                            &m_uPCMLoopStart,   // Beginning of loop offset.
                            &m_uPCMLoopEnd,		// End of loop offset.
							&m_ulDataSize,		// Data size.
							&m_ulDataOffset,	// Offset to data.
							&out_xmaCustomData, // Custom XMA data.
							sizeof( AkXMA2CustomData ) );

    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_InvalidFile;
    }
	AKASSERT( m_ulDataOffset % 2048 == 0 );

    const BYTE & uChannels = out_xmaFmt.Streams[0].Channels; // & fmt2.Streams[0].ChannelMask;

	// Verify consistency with bank/WAL info.
	/** FIXME: Banks and XMA headers are completely inconsistent! See WG-9838.
#ifdef _DEBUG
	AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( pFormat->uSampleRate == out_xmaFmt.SampleRate 
			&& pFormat->uChannelMask == out_xmaFmt.Streams[0].ChannelMask
			&& pFormat->GetNumChannels() == out_xmaFmt.Streams[0].Channels );
#endif
	**/

	AKASSERT( !( m_ulDataSize & 2047 ) ); // Data should be a multiple of 2K

	m_uiTotalOutputSamples = out_xmaFmt.SamplesEncoded;

    // XMA2? then keep PCM loop points (data boundaries otherwise).
    if ( out_xmaFmt.LoopEnd )
    {
        m_uPCMLoopStart = out_xmaFmt.LoopBegin;
        m_uPCMLoopEnd   = out_xmaFmt.LoopEnd;
    }
    else
    {
        m_uPCMLoopStart = 0;
        m_uPCMLoopEnd   = m_uiTotalOutputSamples;
    }

	// Init XMA decoder

	AkUInt32 uiOutputBufferSize = min( XMADECODER_MAX_OUTPUT_BUFFER_SIZE_IN_BYTES, 
		XMA_POOL_ALLOC_SIZE - ( XMADECODER_SUBFRAME_SIZE_IN_BYTES * uChannels ) );

	XMA_PLAYBACK_INIT init;

	/// IMPORTANT: Must use the header's sample rate because banks can store invalid values (see WG-9838).
	init.sampleRate = out_xmaFmt.SampleRate;
	init.outputBufferSizeInSamples = uiOutputBufferSize / ( XMADECODER_SAMPLE_SIZE_IN_BYTES * uChannels );
	init.channelCount = uChannels;
	init.subframesToDecode = SUBFRAMES_TO_DECODE;

	AkUInt32 uiDecodingBufferSize = XMAPlaybackGetRequiredBufferSize( 1, &init );
	AKASSERT( !m_pXMADecodingBuffer );
	m_pXMADecodingBuffer = (AkUInt8*)AK::MemoryMgr::Malign( g_LEngineDefaultPoolId, uiDecodingBufferSize, XMADECODER_INPUT_BUFFER_ALIGNMENT );// Must be aligned on 2k
	if( !m_pXMADecodingBuffer )
		return AK_InsufficientMemory;

	HRESULT hr = XMAPlaybackCreate( 1, &init, XMA_PLAYBACK_CREATE_USE_PROVIDED_MEMORY, &m_pPlayback, m_pXMADecodingBuffer, uiDecodingBufferSize );
	if ( hr == S_OK )
	{
		hr = XMAPlaybackRequestModifyLock( m_pPlayback );
		if ( hr == S_OK )
			hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
	}
	if ( hr != S_OK ) 
	{
		StopStream();
		return AK_Fail;
	}

    hr = XMAPlaybackSubmitData( m_pPlayback, 0, (char *) in_pBuffer + m_ulDataOffset, m_ulDataSize );
	AKASSERT( hr == S_OK );
	if ( hr != S_OK ) 
	{
		StopStream();
		return AK_Fail;
	}
    return AK_Success;
}

void CAkSrcBankXMA::VirtualSeek( AkUInt32& io_uSampleOffset, XMA2WAVEFORMAT& in_xmaFmt, AkUInt32 in_uSeekTableOffset, AkUInt8* pData )
{
	// Find block in seek table. (Navigate from beginning if not XMA2).
    int i = 0;
    DWORD * pSeekTable = NULL;
    if ( in_xmaFmt.BlockCount )
    {
        pSeekTable = (DWORD*)( pData + in_uSeekTableOffset );
        while ( io_uSampleOffset > pSeekTable[i] )
            ++i;
    }
    pData += m_ulDataOffset;

    DWORD dwBitOffset = 8 * i*in_xmaFmt.BlockSizeInBytes;
    // Now, navigate through XMA data to find correct offset (will round down to 128 samples boundary).
    
    DWORD dwSubframesSkip;
    if ( AK::XMAHELPERS::FindDecodePosition( pData, m_ulDataSize, ( io_uSampleOffset-(i?pSeekTable[i-1]:0) ), dwBitOffset, dwSubframesSkip ) == AK_Success )
    {
        HRESULT hr = XMAPlaybackSetDecodePosition( m_pPlayback, 0, dwBitOffset, dwSubframesSkip );   
        AKASSERT( hr == S_OK );
		AkUInt32 uRoundingError = io_uSampleOffset % XMADECODER_SUBFRAME_SIZE_IN_SAMPLES;
		io_uSampleOffset = io_uSampleOffset - uRoundingError;
    }
    else
	{
		io_uSampleOffset = 0;
        AKASSERT( !"Could not find a decoder offset that matches the current virtual sample" );
	}
}
