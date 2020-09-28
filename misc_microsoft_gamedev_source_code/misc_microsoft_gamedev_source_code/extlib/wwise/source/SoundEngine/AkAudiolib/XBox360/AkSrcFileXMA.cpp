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

#include "AkSrcFileXMA.h"

#include "AudiolibDefs.h"
#include "AkFileParser.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"

#include "xauddefs.h"
#include "XMADecoder.h"
#include <stdio.h>
#include <tchar.h>

#include "AkXMAHelpers.h"
#include "AkLEngine.h"


CAkSrcFileXMA::CAkSrcFileXMA( CAkPBI * in_pCtx )
	: CAkSrcBaseEx( in_pCtx )
	, m_pPlayback( NULL )
	, m_pStream( NULL )
	, m_bEndReached( false )
    , m_uNumBlocks( 0 )
    , m_arSeekTable( NULL )
	, m_pOutBuffer( NULL )
	, m_pXMADecodingBuffer( NULL )
	, m_uDidLoop( 0 )
{
    m_bIsFirstBufferPending = false;

	m_buffers[ 0 ].pXMAData = NULL;
	m_buffers[ 0 ].pToRelease = NULL;
	m_buffers[ 1 ].pXMAData = NULL;
	m_buffers[ 1 ].pToRelease = NULL;
}

CAkSrcFileXMA::~CAkSrcFileXMA()
{
	ReleaseBuffer();
}

AKRESULT CAkSrcFileXMA::StartStream()
{
    // StartStream() should not be called if it is ready and running.
	AKASSERT( m_pStream == NULL || m_bIsFirstBufferPending );

    // Try process first buffer if stream is already created and running
    if ( m_bIsFirstBufferPending )
    {
        AKASSERT( m_pStream != NULL );

		// NOTE. ProcessFirstBuffer() returns AK_FormatNotReady if it had time to parse the header, but
		// could not submit any data to the decoder. In such a case the context was acquired and we are 
		// just trying to refill it.
		if ( m_pPlayback )
		{
			if ( !(m_buffers[ 0 ].pXMAData) )
			{
				// no data was submitted yet.

				HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
				AKASSERT( hr == S_OK );
				hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
				AKASSERT( hr == S_OK );

				RefillInputBuffers();            

				hr = XMAPlaybackResumePlayback( m_pPlayback );
				AKASSERT( hr == S_OK );
			}

			// We are finally ready if some data have been decoded.
			return ( XMAPlaybackQueryAvailableData( m_pPlayback, 0 ) > 0 ) ? AK_Success : AK_FormatNotReady;

		}
        return ProcessFirstBuffer( );
    }

	m_bIsFirstBufferPending = true;

	AKRESULT eResult;

    // Get audio context data for stream settings.
    
    AkSrcDescriptor srcDesc;
    eResult = m_pCtx->GetSrcDescriptor( &srcDesc );
    if ( eResult != AK_Success )
    {
        return AK_Fail;
    }

    m_uIOBlockSizeCorr = 0;

	AKASSERT( AK::IAkStreamMgr::Get( ) );

    // Stream heuristics.
    AkAutoStmHeuristics heuristics;
    heuristics.uMinNumBuffers = 2;
    // Average throughput: taken from format (updated in ParseHeader() with real format. 
    heuristics.fThroughput = 16.384;	// We can only guess: 16 Kb/s.
    // Looping: If not looping, set both values to 0. Otherwise, set loop end to end of file until header is parsed.
    heuristics.uMinNumBuffers  = 3;
    heuristics.uLoopStart = 0;
    heuristics.uLoopEnd = 0;    // Never looping.
    // Priority.
    heuristics.priority = m_pCtx->GetPriority( );

    // Buffer settings (required by decoding hardware constraints).
    AkAutoStmBufSettings bufSettings;
    bufSettings.uBufferSize     = 0;    // Do not set buffer size explicitly.
    
    bufSettings.uMinBufferSize  = XMADECODER_INPUT_BUFFER_ALIGNMENT * 4;  // Min buffer size.
    bufSettings.uBlockSize      = XMADECODER_INPUT_BUFFER_ALIGNMENT * 2;  // Req block size.
    
	// Generation of complete file name from bank encoded name and global audio source path settings
    // is done at the file system level. Just fill up custom FS parameters to give it a clue.
    AkFileSystemFlags fileSystemFlags = 
    {
        AKCOMPANYID_AUDIOKINETIC,               // Company ID. AK uses AKCOMPANY_ID_AUDIOKINETIC (defined in AkTypes.h).
        CODECID_FROM_PLUGINID( srcDesc.uiID ),  // File/Codec type ID (defined in AkTypes.h).
        NULL,               // User parameter size.
        0,                  // User parameter.
        (srcDesc.bIsLanguageSpecific) // True when file location depends on current language.
    };

    // Open.
    // Use string overload if pszFilename is set (would be set by the WAL, in !NOWWISE builds only).
    // Otherwise use ID overload.
#ifndef AK_OPTIMIZED
    if ( srcDesc.pvPath == NULL )
    {
        AKASSERT( srcDesc.ulFileID != AK_INVALID_FILE_ID );
        eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                    srcDesc.ulFileID,   // Application defined ID.
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Heuristics.
                                    &bufSettings,       // Stream buffers settings.
                                    m_pStream );        // IAkAutoStream ** out_ppStream.
    }
    else
    {
        eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                    (AkLpCtstr)srcDesc.pvPath,  // Application defined string (title only, or full path, or code...).
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Heuristics.
                                    &bufSettings,       // Stream buffers settings.
                                    m_pStream );        // IAkAutoStream ** out_ppStream.
    }
#else
    eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                srcDesc.ulFileID,   // Application defined string (title only, or full path, or code...).
                                &fileSystemFlags,   // File system special parameters.
                                heuristics,         // Heuristics.
                                &bufSettings,       // Stream buffers settings.
                                m_pStream );        // IAkAutoStream ** out_ppStream.
#endif

    if ( eResult != AK_Success )
    {
        AKASSERT( m_pStream == NULL );
        //AKASSERT( !"Could not open stream" ); Monitored at a lower level.
        return eResult;
    }

    // In profiling mode, name the stream.
    // Profiling: create a string out of FileID.
#ifndef AK_OPTIMIZED
    if ( srcDesc.pvPath != NULL )
        m_pStream->SetStreamName( (AkLpCtstr)srcDesc.pvPath );
    else
    {
        const unsigned long MAX_NUMBER_STR_SIZE = 11;
        AkTChar szName[MAX_NUMBER_STR_SIZE];
        _stprintf( szName, _T("%u"), srcDesc.ulFileID );
        m_pStream->SetStreamName( szName );
    }
#endif

    if ( m_pCtx->IsPrefetched() && !m_pCtx->GetSourceOffset() )
	{
		AkUInt8 * pPrefetch;
		AkUInt32  uiPrefetchSize;

		m_pCtx->GetDataPtr( pPrefetch, uiPrefetchSize );
		if ( pPrefetch && uiPrefetchSize > 0 )
		{
			// Prefetch size must be a multiple of 2048. Force it.
			uiPrefetchSize &= ~( 2047 );
			AKASSERT( uiPrefetchSize > 0 );

			eResult = ParseHeader( pPrefetch, uiPrefetchSize );
			if ( eResult != AK_Success )
				return eResult;

			// Skip prefetch.
			AkInt64 lRealPosition, lSizeMove;
			lSizeMove = uiPrefetchSize;
			if ( m_pStream->SetPosition( lSizeMove, AK_MoveBegin, &lRealPosition ) != AK_Success )
			{
				AKASSERT( !"Could not set file position" );
				return AK_Fail;
			}
			m_uIOBlockSizeCorr = AkUInt32( lSizeMove - lRealPosition );

			// Create XMA context.
			eResult = CreateXMAContext();

			if ( eResult == AK_Success )
			{
				// Submit the rest of the prefetch data if any.
				AkUInt32 uFirstDataSize = ( uiPrefetchSize - m_uDataOffset );
				AKASSERT( !( uFirstDataSize & 2047 ) );

				if ( uFirstDataSize > 0 )
					eResult = SubmitFirstBuffer( pPrefetch + m_uDataOffset, uFirstDataSize );
				else
				{
					// No data to submit: The source is not ready to output samples.
					eResult = AK_FormatNotReady;				
				}

				m_pStream->Start();
			}
			
			return eResult;
		}
	}

	m_pStream->Start();
    return ProcessFirstBuffer( );
}

void CAkSrcFileXMA::StopStream()
{
	// Clean up.
	
	// Note: XMA does not derive from SrcFileBase
	if ( m_pStream != NULL )
    {
        m_pStream->Destroy( );
        m_pStream = NULL;
    }
    
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
		AkFree( g_LEngineDefaultPoolId, m_pXMADecodingBuffer );
		m_pXMADecodingBuffer = NULL;
	}

	if ( m_arSeekTable )
	{
		AkFree( g_LEngineDefaultPoolId, m_arSeekTable );
		m_arSeekTable = NULL;
	}

	CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcFileXMA::StopLooping()
{
    m_uLoopCnt = 1;

	if ( m_pStream )
	{
		// Set stream's heuristics to non-looping.
		AkAutoStmHeuristics heuristics;
		m_pStream->GetHeuristics( heuristics );
		heuristics.uLoopEnd = 0;
		m_pStream->SetHeuristics( heuristics );
	}

	return AK_Success;
}

void CAkSrcFileXMA::GetBuffer( AkVPLState & io_state )
{	
    AKASSERT( m_pPlayback );
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

	AkAudioFormat* pFormat = m_pCtx->GetMediaFormat();

	DWORD nReqSamples = io_state.buffer.MaxFrames();
	AKASSERT( nReqSamples >= XMADECODER_SUBFRAME_SIZE_IN_SAMPLES );

    AkUInt8 * pOutBuffer = NULL;
    DWORD dwConsumed;
	AkUInt32 l_ulCurrentOffset = m_uDecodedSmpls; //keep a copy of the current position for markers
	io_state.result = ConsumeData( nReqSamples,
                      dwConsumed,
                      pOutBuffer );

	if ( RefillInputBuffers( ) == AK_Fail )
		io_state.result = AK_Fail;

	if( io_state.result == AK_NoMoreData ||  io_state.result == AK_DataReady )
	{
		// Allocate output buffer for decoded data (we don't want to hold XMA hardware decoder's output buffer too long
		AKASSERT( !m_pOutBuffer );
		AkUInt32 uDecodedBufSize = dwConsumed * pFormat->GetNumChannels() * sizeof(AkInt16);
		m_pOutBuffer = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( LE_MAX_FRAMES_PER_BUFFER * pFormat->GetNumChannels() * sizeof(AkInt16) ); 
		if ( !m_pOutBuffer )
		{
			hr = XMAPlaybackResumePlayback( m_pPlayback );
			AKASSERT( hr == S_OK );
			io_state.result = AK_InsufficientMemory;
			return;
		}
		AKPLATFORM::AkMemCpy( m_pOutBuffer, pOutBuffer, uDecodedBufSize );

		if ( m_bEndReached )
		{
			if ( XMAPlaybackQueryAvailableData( m_pPlayback, 0 ) == 0 && 
				 XMAPlaybackIsIdle( m_pPlayback, 0 ) )
			{
				io_state.result = AK_NoMoreData;
			}
		}
	}

	hr = XMAPlaybackResumePlayback( m_pPlayback );
	AKASSERT( hr == S_OK );

	AkUInt16 uNumFrames = (AkUInt16)( dwConsumed );
	io_state.buffer.AttachInterleavedData( m_pOutBuffer, LE_MAX_FRAMES_PER_BUFFER, uNumFrames, pFormat->GetChannelMask() );

	bool bDidLoop = false;
	if( m_uDidLoop )
	{
		if( l_ulCurrentOffset + io_state.buffer.uValidFrames >= m_uiTotalOutputSamples )
		{
			bDidLoop = true;
			--m_uDidLoop;
		}
	}
	CopyRelevantMarkers( io_state.buffer, l_ulCurrentOffset, bDidLoop );

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = pFormat->uSampleRate;
		io_state.buffer.posInfo.uStartPos = l_ulCurrentOffset;
		io_state.buffer.posInfo.uFileEnd = m_uiTotalOutputSamples;
	}
}

void CAkSrcFileXMA::ReleaseBuffer()
{
	if ( m_pOutBuffer )
	{
		CAkLEngine::ReleaseCachedAudioBuffer( LE_MAX_FRAMES_PER_BUFFER * m_pCtx->GetMediaFormat()->GetNumChannels() * sizeof(AkInt16), m_pOutBuffer );
		m_pOutBuffer = NULL;
	}
}

AKRESULT CAkSrcFileXMA::SubmitData( 
    AkUInt32 in_uiSizeRead, 
    AkUInt8 * in_pData
    )
{
    AKASSERT( in_uiSizeRead > 0 );

    // Remember XMA data position at beginning of this buffer.
    AkUInt32 uCurOffset = m_uXMADataPosition;

    // Update offset in file.
    m_uXMADataPosition += in_uiSizeRead;

    // Will we hit the loopback boundary?
    if ( m_uXMADataPosition >= m_uDataSize )
    {
		if ( DoLoop() )
		{
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
			++m_uDidLoop;
			
            // Seek in file.
            // -------------

            // Set position for next I/O read.
            AkInt64 lRealOffset, lSizeMove;
            
            // Find file position to seek to. It is the beginning of the 2K aligned address 
            // that is before m_dwLoopStartOffset.
            // TEMP. Always beginning of data.
            lSizeMove = m_uDataOffset;

            // Set stream position now for next read.
            if ( m_pStream->SetPosition(
                        lSizeMove,          // Seek offset.
                        AK_MoveBegin,       // Seek method, from beginning, end or current file position.
                        &lRealOffset        // Actual seek offset may differ from expected value when unbuffered IO.
                                            // In that case, floors to sector boundary. Pass NULL if don't care.
                        ) != AK_Success )
            {
                AKASSERT( !"Error while setting stream position" );
                return AK_Fail;
            }

            // Keep track of offset caused by unbuffered IO constraints.
            m_uIOBlockSizeCorr = (AkUInt32)( lSizeMove - lRealOffset );
            // Set file offset to true value.
            m_uXMADataPosition = (AkUInt32)lSizeMove - m_uDataOffset;  
            
        }            
        else if ( m_uLoopCnt == 1 )
        {
        	// Set stream's heuristics to non-looping.
            AkAutoStmHeuristics heuristics;
            m_pStream->GetHeuristics( heuristics );
            heuristics.uLoopEnd = 0;
            m_pStream->SetHeuristics( heuristics );
            
            // Hit the end of file: will be NoMoreData.
            // Set this flag to notify output.
            m_bEndReached = true;
        }
    }

    // Submit data now.
    HRESULT hr = XMAPlaybackSubmitData( m_pPlayback, 0, in_pData, in_uiSizeRead );
    AKASSERT( hr == S_OK );
    return ( hr == S_OK ) ? AK_Success : AK_Fail;
}

AKRESULT CAkSrcFileXMA::ConsumeData( 
    DWORD in_nReqSamples,
    DWORD & out_nSamplesConsumed,
    AkUInt8 *& out_pBuffer )
{
    out_nSamplesConsumed = XMAPlaybackConsumeDecodedData( m_pPlayback, 0, in_nReqSamples, (void **) &out_pBuffer );
	if ( out_nSamplesConsumed == 0 )
	{
		// No data available. Check if there was an error.
		/** NOTE: Cannot check if there was an error. Errors occur when streaming starves, or when 
		we stop feeding the decoder before becoming virtual (FromElapsedTime).
		(Interestingly the decoder resets its error status when we start feeding it again).
		if ( XMAPlaybackGetParseError( m_pPlayback, 0 ) != XMA_PLAYBACK_PARSE_ERROR_NONE 
			|| XMAPlaybackGetErrorBits( m_pPlayback, 0 ) != XMA_PLAYBACK_ERROR_NONE )
		{
			return AK_Fail;
		}
		**/
	    return AK_NoDataReady;
	}
    m_uDecodedSmpls += out_nSamplesConsumed;

    if ( m_uDecodedSmpls >= m_uiTotalOutputSamples /*m_uLoopBack */ )
        m_uDecodedSmpls -= m_uiTotalOutputSamples; // m_uLoopBack
    return AK_DataReady;
}

void CAkSrcFileXMA::ReleaseAllBuffers()
{
    for ( int iBuffer = 0; iBuffer < 2; ++iBuffer ) 
	{
		Buffer & buffer = m_buffers[ iBuffer ];
        if ( buffer.pToRelease )
		{
			m_pStream->ReleaseBuffer(  );
			buffer.pToRelease = NULL;
		}
		buffer.pXMAData = NULL;
	}
}

void CAkSrcFileXMA::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if( m_pStream )
	{
		m_pStream->Stop();
		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
		{
			m_uXMADataPosition = 0;
			m_uLoopCnt = m_pCtx->GetLooping();
			m_uDecodedSmpls = 0;
			m_bEndReached = false; 
			
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
				AkFree( g_LEngineDefaultPoolId, m_pXMADecodingBuffer );
				m_pXMADecodingBuffer = NULL;
			}
			
			// Release all buffers.
			ReleaseAllBuffers( );

			// Seek to beginning of data.
			AkInt64 lRealOffset, lSizeMove = m_uDataOffset;
			if ( m_pStream->SetPosition(
						lSizeMove,          // Seek offset.
						AK_MoveBegin,       // Seek method, from beginning, end or current file position.
						&lRealOffset        // Actual seek offset may differ from expected value when unbuffered IO.
											// In that case, floors to sector boundary. Pass NULL if don't care.
						) != AK_Success )
			{
				AKASSERT( !"Error while setting stream position" );
			} 
            m_uIOBlockSizeCorr = (AkUInt32)( lSizeMove - lRealOffset );
		}
        /* From now on, we stop managing the input side.
	    else if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	    {
	    }
        */
    }
}

AKRESULT CAkSrcFileXMA::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
    AKRESULT eResult = m_pStream->Start();
	if( eResult != AK_Success )
		return eResult;
	
	if ( !m_pPlayback )
	{
		// Virtual seek required with XMA2 FromElapsedTime behavior.
        if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		{
			AKASSERT( m_arSeekTable && m_uXMA2BlockSize && m_uNumBlocks );
			eResult = VirtualSeek( m_uDecodedSmpls );
			if ( eResult != AK_Success )
				return eResult;
		}

		// XMA context should have been released with 
		// AkVirtualQueueBehavior_FromBeginning and AkVirtualQueueBehavior_FromElapsedTime 
		// virtual behaviors only, although it could have been kept in the latter case:
		// the decoder does not become idle with XMA1 files (that don't have seek tables).
		AKASSERT( eBehavior == AkVirtualQueueBehavior_FromElapsedTime ||
				  eBehavior == AkVirtualQueueBehavior_FromBeginning );
		
		eResult = CreateXMAContext();
        if ( eResult == AK_Success )
		{
			// Try refilling input buffers now.
			HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
			AKASSERT( hr == S_OK );
			hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
			AKASSERT( hr == S_OK );

			RefillInputBuffers( );            

			hr = XMAPlaybackResumePlayback( m_pPlayback );
			AKASSERT( hr == S_OK );
		}
    }
	
	return eResult;
}

AKRESULT CAkSrcFileXMA::TimeSkip( AkUInt32 & io_uFrames )
{
	AKRESULT eResult = AK_DataReady;
	AkUInt32 uPosBeforeSkip = m_uDecodedSmpls;
    if ( !m_uNumBlocks )
    {
        // XMA 1: Get and release buffers.
		return CAkVPLSrcNode::TimeSkip( io_uFrames );
    }
	else if ( m_pPlayback )
    {
        // Consume data as long as it is ready.
        AKASSERT( m_pPlayback );
	    HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
	    AKASSERT( hr == S_OK );
	    hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
	    AKASSERT( hr == S_OK );
    	
        AkUInt8 * pDummy;
        DWORD dwConsumedSamples;
        eResult = ConsumeData( io_uFrames,
                               dwConsumedSamples,
                               pDummy );
        
        // Release buffers without getting more.
        for ( int iBuffer = 0; iBuffer < 2; ++iBuffer ) 
		{
			Buffer & buffer = m_buffers[ iBuffer ];

			if ( buffer.pXMAData && !XMAPlaybackQueryInputDataPending( m_pPlayback, 0, buffer.pXMAData ) )
			{
				if ( buffer.pToRelease )
				{
					m_pStream->ReleaseBuffer(  );
					buffer.pToRelease = NULL;
				}

				buffer.pXMAData = NULL;
			}
        }
        
        io_uFrames = dwConsumedSamples;
        
        if ( eResult == AK_DataReady ||
            eResult == AK_NoMoreData )
        {
			AKVERIFY( XMAPlaybackResumePlayback( m_pPlayback ) == S_OK );
			TimeSkipMarkers( uPosBeforeSkip, io_uFrames, m_uiTotalOutputSamples );
            return eResult;
        }

        // No data ready. Destroy playback.
        AKVERIFY( XMAPlaybackDestroy( m_pPlayback ) == S_OK );
        m_pPlayback = NULL;
		if( m_pXMADecodingBuffer )
		{
			AkFree( g_LEngineDefaultPoolId, m_pXMADecodingBuffer );
			m_pXMADecodingBuffer = NULL;
		}

        ReleaseAllBuffers( );
    }

    // Otherwise keep count of samples.

    AKASSERT( !m_pPlayback &&
              m_buffers[0].pToRelease == NULL &&
              m_buffers[1].pToRelease == NULL );
    
    m_uDecodedSmpls += io_uFrames;

    // Handle looping
    if ( m_uDecodedSmpls >= m_uiTotalOutputSamples )
    {
        if ( DoLoop() )
        {
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
            m_uDecodedSmpls -= m_uiTotalOutputSamples;
            if ( m_uLoopCnt == 1 )
            {
                // Set stream's heuristics to non-looping.
                AkAutoStmHeuristics heuristics;
                m_pStream->GetHeuristics( heuristics );
                heuristics.uLoopEnd = 0;
                m_pStream->SetHeuristics( heuristics );
            }
        }
        else if ( m_uLoopCnt == 1 )
        {
            io_uFrames -= (m_uDecodedSmpls - m_uiTotalOutputSamples);
            eResult = AK_NoMoreData;
        }
    }
	TimeSkipMarkers( uPosBeforeSkip, io_uFrames, m_uiTotalOutputSamples );
    return eResult;
}

// Process first buffer from stream: 
// If not ready, returns AK_FormatNotReady.
// Otherwise, 
// - calls parse header;
// - sets m_bIsReady flag.
AKRESULT CAkSrcFileXMA::ProcessFirstBuffer( )
{
	AkUInt32 uiBufferSize;
	AkUInt8 * pStmBuffer;
    AKRESULT eResult = m_pStream->GetBuffer(
                            (void*&)pStmBuffer,	// Address of granted data space.
                            uiBufferSize,		// Size of granted data space.
                            false );
    
    if ( eResult == AK_NoDataReady )
    {
        // Not ready. Leave.
        return AK_FormatNotReady;
    }
    else if ( eResult != AK_DataReady &&
              eResult != AK_NoMoreData )
    {
        AKASSERT( !"Could not read stream buffer" );
        return AK_Fail;
    }

    // Parse header. 
    eResult = ParseHeader( pStmBuffer, uiBufferSize );
	if ( eResult != AK_Success )
		return eResult;

	eResult = CreateXMAContext();
	if ( eResult != AK_Success )
		return eResult;

	AkUInt32 uiFirstDataSize = ( uiBufferSize - m_uDataOffset );
	AKASSERT( !( uiFirstDataSize & 2047 ) );

	// Handle source offset.
	AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
	if( l_numNativeSampleOffset )
	{
		AkUInt32 l_numRealSampleOffset = (AkUInt32)( l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE );

		m_pCtx->SetSourceOffset( l_numRealSampleOffset % XMADECODER_SUBFRAME_SIZE_IN_SAMPLES );
		eResult = VirtualSeek( l_numRealSampleOffset );
		if ( eResult != AK_Success )
			return eResult;

		// We just changed the StreamMgr position. Don't even bother trying to get a buffer...
		// We have no data to submit now.
		uiFirstDataSize = 0;
	}

	if ( uiFirstDataSize > 0 )
	{
		// Some audio data is ready to be submitted. The source is ready (to be connected).
		m_buffers[0].pToRelease = pStmBuffer;
		m_bIsFirstBufferPending = false;
		eResult = SubmitFirstBuffer( pStmBuffer + m_uDataOffset, uiFirstDataSize );
	}
	else
	{
		// No data ready to submit.

		// Release our first stream buffer now.
		AKVERIFY( m_pStream->ReleaseBuffer() == AK_Success );

		// The source is not ready to output samples.
		eResult = AK_FormatNotReady;
	}

	return eResult;
}

AKRESULT CAkSrcFileXMA::SubmitFirstBuffer( 
	AkUInt8 * in_pBuffer,
	AkUInt32 in_uFirstDataSize
	)
{
	HRESULT hr = XMAPlaybackRequestModifyLock( m_pPlayback );
	if ( hr == S_OK )
	{
		hr = XMAPlaybackWaitUntilModifyLockObtained( m_pPlayback );
		AKASSERT( hr == S_OK );
		if ( hr != S_OK ) 
		{
			StopStream();
			return AK_Fail;
		}

		m_buffers[ 0 ].pXMAData = in_pBuffer;

		AKRESULT eResult = SubmitData( in_uFirstDataSize, in_pBuffer );
		if ( eResult == AK_Success )
		{
			hr = XMAPlaybackResumePlayback( m_pPlayback );
			AKASSERT( hr == S_OK );
		}
		return eResult;
	}
	return AK_Fail;
}

AKRESULT CAkSrcFileXMA::RefillInputBuffers( )
{
    AKRESULT eReturn = AK_Success;
    for ( int iBuffer = 0; iBuffer < 2; ++iBuffer ) 
	{
		Buffer & buffer = m_buffers[ iBuffer ];

		if ( buffer.pXMAData && !XMAPlaybackQueryInputDataPending( m_pPlayback, 0, buffer.pXMAData ) )
		{
			if ( buffer.pToRelease )
			{
				m_pStream->ReleaseBuffer(  );
				buffer.pToRelease = NULL;
			}

			buffer.pXMAData = NULL;
		}

        // Try submit incomplete buffer instead, if it is set.
		if ( !buffer.pXMAData )
		{
			AkUInt32 uiSize = 0;

            AKRESULT eResult = m_pStream->GetBuffer( (void *&) buffer.pToRelease, uiSize, false );
			if ( eResult != AK_NoMoreData &&
                    eResult != AK_NoDataReady && 
                    eResult != AK_DataReady )
			{
				eReturn = AK_Fail;
			}
            
			if ( uiSize )
			{
                AKASSERT( m_uIOBlockSizeCorr < uiSize );
                buffer.pXMAData = buffer.pToRelease + m_uIOBlockSizeCorr;
                uiSize -= m_uIOBlockSizeCorr;
                m_uIOBlockSizeCorr = 0;


                // Do we need to set the position because of a TimeElapsed virtual behavior?
                if ( !m_uSamplesToSkip )
                {
                    // No. Submit data.
                    eReturn = SubmitData( uiSize, buffer.pXMAData );
                }
                else
                {
                    // Yes. Find it.
                    AkUInt8 * pData = buffer.pXMAData;

                    // In this particular case, playback shoud be idle. 
                    AKASSERT( XMAPlaybackIsIdle( m_pPlayback, 0 ) );

                    // Now, navigate through XMA data to find correct offset (will round down to 128 samples boundary).
                    DWORD dwBitOffset = 0;
                    DWORD dwSubframesSkip;
                    if ( AK::XMAHELPERS::FindDecodePosition( pData, uiSize, m_uSamplesToSkip, dwBitOffset, dwSubframesSkip ) == AK_Success )
                    {
                        // Offset was found in this buffer. Submit and set position.
                        eReturn = SubmitData( uiSize, pData );
                        if ( eReturn == AK_Success )
                        {
                            HRESULT hr = XMAPlaybackSetDecodePosition( m_pPlayback, 0, dwBitOffset, dwSubframesSkip );   
                            AKASSERT( hr == S_OK );

                            m_uSamplesToSkip = 0;
                        }
                    }
                    else
                    {
                        // Offset was not found in this buffer. Give some time to the StreamMgr.
                        // Release it.
                        if ( buffer.pToRelease )
				        {
					        m_pStream->ReleaseBuffer(  );
					        buffer.pToRelease = NULL;
				        }
				        buffer.pXMAData = NULL;

                        m_uSamplesToSkip -= dwSubframesSkip * XMADECODER_SUBFRAME_SIZE_IN_SAMPLES;
                    }
                }                
			}
		}
    } 
    return eReturn;
}

AkTimeMs CAkSrcFileXMA::GetDuration( void ) const
{
	// maths must be done in float to avoid overflow fix WG-1849
    return (AkTimeMs) ( ( 1000.f * (AkReal32)m_pCtx->GetLooping() * (AkReal32)m_uiTotalOutputSamples ) / (AkReal32)m_pCtx->GetMediaFormat()->uSampleRate );
}

AKRESULT CAkSrcFileXMA::ParseHeader( AkUInt8 * in_pBuffer, AkUInt32 in_uiSize )
{
    XMA2WAVEFORMAT xmaFmt;
	AkXMA2CustomData xmaCustomData;

	AkUInt32 uiDataSize, uiDataOffset;

    xmaCustomData.uSeekTableOffset = 0;

	AKRESULT eResult = CAkFileParser::Parse( in_pBuffer,      // Data buffer
                            in_uiSize,  // Buffer size
                            (WaveFormatEx *) &xmaFmt,        // Returned audio format.
							sizeof( XMA2WAVEFORMAT ),
                            &m_markers,			// Markers.
                            &m_uPCMLoopStart,   // Beginning of loop offset.
                            &m_uPCMLoopEnd,		// End of loop offset.
                            &uiDataSize,		// Data size.
							&uiDataOffset,		// Offset to data.
							&xmaCustomData,		// Custom XMA data.
							sizeof( AkXMA2CustomData ) );		

    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_InvalidFile;
    }
	AKASSERT( uiDataOffset % 2048 == 0 );

	AKASSERT( !( uiDataSize & 2047 ) ); // Data should be a multiple of 2K

    const BYTE & uChannels = xmaFmt.Streams[0].Channels; // & fmt2.Streams[0].ChannelMask;

	// Verify consistency with bank/WAL info.
	/** FIXME: Banks and XMA headers are completely inconsistent! See WG-9838.
#ifdef _DEBUG
	AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( pFormat->uSampleRate == xmaFmt.SampleRate 
			&& pFormat->uChannelMask == xmaFmt.Streams[0].ChannelMask
			&& pFormat->GetNumChannels() == xmaFmt.Streams[0].Channels );
#endif
	**/

	// Init XMA decoder

	AkUInt32 uiOutputBufferSize = min( XMADECODER_MAX_OUTPUT_BUFFER_SIZE_IN_BYTES, 
		XMA_POOL_ALLOC_SIZE - ( XMADECODER_SUBFRAME_SIZE_IN_BYTES * uChannels ) );

	// PsuedoBytesPerSec is actually the number of frames thanks to the conversion plugin's preprocessing
    m_uiTotalOutputSamples = xmaFmt.SamplesEncoded;

	// Looping in XMA file is not yet supported, filling LoopStart and LoopEnd with file boundaries.
	m_uPCMLoopStart = 0;
	m_uPCMLoopEnd = m_uiTotalOutputSamples;

    // File data.
    m_uDataOffset  = uiDataOffset;
    m_uDataSize    = uiDataSize;

    AKASSERT( uiDataOffset <= in_uiSize );
    m_uXMADataPosition   = 0;

    m_uDecodedSmpls = 0;
    m_uLoopCnt = m_pCtx->GetLooping();

    // Fill seek table. Don't care if XMA1.
    m_uNumBlocks = xmaFmt.BlockCount;
    m_uXMA2BlockSize = xmaFmt.BlockSizeInBytes;
    AKASSERT( m_arSeekTable == NULL );
    if ( m_uNumBlocks )
    {
        if ( xmaCustomData.uSeekTableOffset == 0 )
        {
            AKASSERT( !"Blocks defined but no seek table" );
            return AK_InvalidFile;
        }
        m_arSeekTable = (AkUInt32*)AkAlloc( g_LEngineDefaultPoolId, m_uNumBlocks*sizeof(AkUInt32) );
        AKPLATFORM::AkMemCpy( m_arSeekTable, in_pBuffer + xmaCustomData.uSeekTableOffset, m_uNumBlocks*sizeof(AkUInt32) );
    }

    m_uSamplesToSkip = 0;

    // Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );
    // Throughput.
	/// IMPORTANT: Must use the header's sample rate because banks can store invalid values (see WG-9838).
	heuristics.fThroughput = ((AkReal32) uiDataSize * xmaFmt.SampleRate ) / ( (AkReal32) m_uiTotalOutputSamples * 1000.f );
    // Priority.
    heuristics.priority = m_pCtx->GetPriority();
    // Looping.
    if ( IS_LOOPING( m_pCtx->GetLooping() ) )
    {
        heuristics.uLoopStart = m_uDataOffset;
        heuristics.uLoopEnd = m_uDataOffset + m_uDataSize;
    }
    
    m_pStream->SetHeuristics( heuristics );

	/// IMPORTANT: Must use the header's sample rate because banks can store invalid values (see WG-9838).
	m_init.sampleRate = xmaFmt.SampleRate;
	m_init.outputBufferSizeInSamples = uiOutputBufferSize / ( XMADECODER_SAMPLE_SIZE_IN_BYTES * uChannels );
	m_init.channelCount = uChannels;
	m_init.subframesToDecode = SUBFRAMES_TO_DECODE;

	return AK_Success;
}

AKRESULT CAkSrcFileXMA::CreateXMAContext()
{
	AKASSERT( m_pPlayback == NULL );
	AKASSERT( !m_pXMADecodingBuffer );
	AkUInt32 uiDecodingBufferSize = XMAPlaybackGetRequiredBufferSize( 1, &m_init );
	m_pXMADecodingBuffer = (AkUInt8*)AK::MemoryMgr::Malign( g_LEngineDefaultPoolId, uiDecodingBufferSize, XMADECODER_INPUT_BUFFER_ALIGNMENT );// Must be aligned on 2k
	if( m_pXMADecodingBuffer )
	{
		HRESULT hr = XMAPlaybackCreate( 1, &m_init, XMA_PLAYBACK_CREATE_USE_PROVIDED_MEMORY, &m_pPlayback, m_pXMADecodingBuffer, uiDecodingBufferSize );
		if ( hr == S_OK )
			return AK_Success;
	}
	return AK_Fail;
}

AKRESULT CAkSrcFileXMA::VirtualSeek( AkUInt32 in_uSamplesToSkip )
{
    // Seek to beginning of block that contains the next requested PCM sample.
    AkUInt32 i = 0;
    while ( in_uSamplesToSkip > m_arSeekTable[i] )
        ++i;
    m_uSamplesToSkip = ( in_uSamplesToSkip-(i?m_arSeekTable[i-1]:0) );
    m_uXMADataPosition = i * m_uXMA2BlockSize;
    AkInt64 lSizeMove = m_uDataOffset + m_uXMADataPosition;
    AkInt64 lRealPosition;
    if ( m_pStream->SetPosition(
                lSizeMove,          // Seek offset.
                AK_MoveBegin,       // Seek method, from beginning, end or current file position.
                &lRealPosition      // Actual seek offset may differ from expected value when unbuffered IO.
                                    // In that case, floors to sector boundary. Pass NULL if don't care.
                ) != AK_Success )
    {
        AKASSERT( !"Error while setting stream position" );
		return AK_Fail;
    }
    m_uIOBlockSizeCorr = (AkUInt32)( lSizeMove - lRealPosition );

    return AK_Success;
}

