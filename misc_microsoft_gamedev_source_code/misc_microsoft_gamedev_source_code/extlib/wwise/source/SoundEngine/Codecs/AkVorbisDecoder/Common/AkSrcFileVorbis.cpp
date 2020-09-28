/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "AkSrcFileVorbis.h"
#include <AK/Plugin/AkVorbisFactory.h>
#include "AkLEngine.h"
#include "AkFileParser.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"
#include "AkVorbisCodec.h"

#define OUTPUT_CHANNEL_SIZE	(LE_MAX_FRAMES_PER_BUFFER * sizeof(AkInt16))

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateVorbisFilePlugin( void* in_pCtx )
{
	#ifdef WIN32
		return AkNew( g_LEngineDefaultPoolId, CAkSrcFileVorbis( (CAkPBI*)in_pCtx, CAkLEngine::SSE2IsAvailable() ) );
	#else
		return AkNew( g_LEngineDefaultPoolId, CAkSrcFileVorbis( (CAkPBI*)in_pCtx ) );
	#endif
}

// Constructor
CAkSrcFileVorbis::CAkSrcFileVorbis( CAkPBI * in_pCtx
#ifdef WIN32									
									, bool in_bSSE2 
#endif									
									) : CAkSrcFileBase( in_pCtx )
{
	// Initially was set to INVALID_POOL_ID, now we know
	g_VorbisPoolId = g_LEngineDefaultPoolId;

	m_uLoopCnt = 0;
	m_uBufferLoopCnt = 0;
	m_pStitchStreamBuffer = NULL;
	m_uStitchBufferLeft = 0;
	m_uStitchBufferValidDataSize = 0;
	m_uStitchBufferEndOffset = 0;
	
#ifdef WIN32
	m_VorbisState.TremorInfo.VorbisDSPState.bSSE2 = in_bSSE2;
#endif	

	// do this here as well as it is legal to be StopStream'ed
	// without having been StartStream'ed
	InitVorbisState();
}

// Destructor
CAkSrcFileVorbis::~CAkSrcFileVorbis()
{
	ReleaseBuffer();
}

// GetBuffer
void CAkSrcFileVorbis::GetBuffer( AkVPLState & io_state )
{
	// The stream should not start before all headers are decoded
	assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );

	// Note: No need to adjust request to for file end or looping as it will finish the decoding a packet anyhow
	m_VorbisState.TremorInfo.uRequestedFrames = io_state.buffer.MaxFrames();

	// Note: We may need to span 2 stream buffer to get sufficient input data. When this occur, we prefer to cache the first buffer
	// to be able to release it right away before getting the second one, rather than run the risk that several stream get into this
	// state and none can advance while waiting for some buffers to be freed.

	if ( m_ulSizeLeft < (m_VorbisState.VorbisInfo.dwMaxPacketSize+sizeof(OggPacketHeader)) )
	{
		// Note: Signal the end of file so decoder can flag packets appropriately
		if (!m_bIsLastStmBuffer)
		{
			if ( m_ulSizeLeft > 0 && m_pStitchStreamBuffer == NULL )
			{
				// Cache left hand side buffer, release it and retrieve a new buffer
				// Allocate enough space to stitch new buffer after and ensure we have enough data not to get out of the cached
				// buffer portion. Worst-case is if you hit the largest packet start at the very end of cached data -> 2*m_VorbisState.VorbisInfo.dwMaxPacketSize will thus suffice.
				m_pStitchStreamBuffer		= (AkUInt8*)AkAlloc( g_VorbisPoolId, (m_VorbisState.VorbisInfo.dwMaxPacketSize+sizeof(OggPacketHeader))+m_ulSizeLeft);
				if ( !m_pStitchStreamBuffer )
				{
					io_state.result = AK_Fail;
					return;
				}
				m_uStitchBufferEndOffset	= m_ulSizeLeft; 
				m_uStitchBufferLeft			= m_ulSizeLeft;	
				m_uStitchBufferValidDataSize = m_ulSizeLeft;
				AkMemCpy( m_pStitchStreamBuffer, m_pNextAddress, m_ulSizeLeft );
				ConsumeData( m_ulSizeLeft );
				AKASSERT( m_ulSizeLeft == 0 );
				m_pNextAddress = NULL;
			}

			if ( m_ulSizeLeft == 0 )
			{
				io_state.result = RetrieveBuffer();
				if ( io_state.result == AK_Fail )
					return;
				else if ( (io_state.result == AK_DataReady || io_state.result == AK_NoMoreData ) && m_pStitchStreamBuffer )
				{
					// Stitch buffer will be used, copy first bit of new stream buffer data to complete stitch buffer
					AkUInt32 uCopySize = AkMin((m_VorbisState.VorbisInfo.dwMaxPacketSize+sizeof(OggPacketHeader)),m_ulSizeLeft);
					AkMemCpy( m_pStitchStreamBuffer+m_uStitchBufferEndOffset, m_pNextAddress, uCopySize );
					m_uStitchBufferLeft += uCopySize;
					m_uStitchBufferValidDataSize += uCopySize;
				}
			}
		}
	}

	// Vorbis is ready, simply decode
	m_VorbisState.uBufferStartPCMFrame = m_VorbisState.uCurrentPCMFrame;

	// No need to decode anything when we have no data to offer
	if( m_pNextAddress == NULL && m_pStitchStreamBuffer == NULL )
	{
		io_state.buffer.uValidFrames	= 0;
		io_state.result 				= AK_NoDataReady;
		return;
	}

	// Allocate output buffer for decoded data
	if ( m_VorbisState.TremorInfo.pucData == NULL )
	{
		m_VorbisState.TremorInfo.pucData = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		if ( m_VorbisState.TremorInfo.pucData == NULL )
		{
			io_state.result = AK_InsufficientMemory;
			return;
		}
	}

	m_VorbisState.TremorInfo.bNoMoreInputPackets = m_bIsLastStmBuffer;
	// Vorbis is ready, simply decode, from the current buffer in use
	if(m_pStitchStreamBuffer)
	{
		m_VorbisState.TremorInfo.uInputDataSize = m_uStitchBufferLeft;
		AkUInt8 * pStitchStreamBufferPos = m_pStitchStreamBuffer + ( m_uStitchBufferValidDataSize - m_uStitchBufferLeft );
		DecodeVorbis( &m_VorbisState.TremorInfo, pStitchStreamBufferPos, (AkInt16*)m_VorbisState.TremorInfo.pucData );
	
		AKASSERT( m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed <= m_uStitchBufferLeft );
		m_uStitchBufferLeft -= m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed;
		AkUInt16 uStitchBufferConsumed = m_uStitchBufferValidDataSize - m_uStitchBufferLeft;
		if ( uStitchBufferConsumed >= m_uStitchBufferEndOffset )
		{
			// May have consumed some data in the current stream buffer as well
			AkUInt32 uSizeConsumed = uStitchBufferConsumed-m_uStitchBufferEndOffset;
			ConsumeData(uSizeConsumed);

			// Consumed all the previous stream buffer part and can stop using stitched data
			AkFree( g_VorbisPoolId, m_pStitchStreamBuffer );
			m_pStitchStreamBuffer = NULL;
			m_uStitchBufferLeft = 0;
			m_uStitchBufferEndOffset = 0;
			m_uStitchBufferValidDataSize = 0;
		}
	}
	else
	{
		AKASSERT( m_pNextAddress );
		m_VorbisState.TremorInfo.uInputDataSize = m_ulSizeLeft;
		DecodeVorbis( &m_VorbisState.TremorInfo, m_pNextAddress, (AkInt16*)m_VorbisState.TremorInfo.pucData );
		ConsumeData(m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed);
	}

	// update frame position
	m_VorbisState.uCurrentPCMFrame += m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced;

	// Set transfer object values.
	io_state.buffer.AttachInterleavedData( 
		m_VorbisState.TremorInfo.pucData, 
		LE_MAX_FRAMES_PER_BUFFER, 
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced,
		m_VorbisState.TremorInfo.uChannelMask );

	if( m_markers.NeedMarkerNotification() )
	{
		// Note: We need to be careful about markers in the looping case. 
		// When looping we do not want to add markers that were not present in the loop on the PCM file, thus only the ls-le region should be considered.
		// fs = file start, els = effective loop start, ls = PCM loop start, le = PCM Loop end, ele = effective loop end, fe = file end
		//(fs)       (els) (ls)       (le)  (ele)      (fe)
		//|| xxxxxxx | xxx | xxxxxxxx | xxx | xxxxxxxx ||
		// Note: uPCMStartLimitMarkers is 0 before first looping occured, uPCMEndLimitMarkers is the loop end untill the last loop iteration is reached.
		
		AkUInt32 uStartFrame = AkMax( m_VorbisState.uPCMStartLimitMarkers, m_VorbisState.uBufferStartPCMFrame );		
		AkUInt32 uEndFrame = AkMin( m_VorbisState.uCurrentPCMFrame, m_VorbisState.uPCMEndLimitMarkers );
		if ( uStartFrame < uEndFrame )
		{
			AkUInt32 uFrameLength = uEndFrame - uStartFrame;
			m_markers.CopyRelevantMarkers( 
				m_pCtx,
				io_state.buffer,
				uStartFrame,
				m_uPCMLoopStart,
				m_uPCMLoopEnd,
				false,
				&uFrameLength );
		}
	}

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = m_VorbisState.uSampleRate;
		io_state.buffer.posInfo.uStartPos = m_VorbisState.uBufferStartPCMFrame;
		io_state.buffer.posInfo.uFileEnd = m_VorbisState.VorbisInfo.dwTotalPCMFrames;
	}

	// Handle looping
	if ( m_VorbisState.uCurrentPCMFrame >= m_VorbisState.uPCMEndLimitMarkers)
	{
		assert( m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset <= m_uPCMLoopStart );
		m_VorbisState.uCurrentPCMFrame = m_uPCMLoopStart - m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset;
		m_VorbisState.uPCMStartLimitMarkers = m_uPCMLoopStart;

		// if not done and not infinite looping
		if(m_uBufferLoopCnt > 1)
		{
			--m_uBufferLoopCnt;
		}
		
		// if done looping
		if(m_uBufferLoopCnt == 1)
		{
			// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
			// so make sure that uCurrentPCMFrame will never be bigger than this one
			m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
		}
	}

	io_state.result = m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus;
}

// ReleaseBuffer
void CAkSrcFileVorbis::ReleaseBuffer()
{
	if ( m_VorbisState.TremorInfo.pucData )
    {
		CAkLEngine::ReleaseCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE, m_VorbisState.TremorInfo.pucData );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		m_VorbisState.TremorInfo.pucData = NULL;
    }
}

// GetDuration
AkTimeMs CAkSrcFileVorbis::GetDuration( ) const
{
	return (AkTimeMs) ((m_VorbisState.VorbisInfo.dwTotalPCMFrames * 1000.f) / m_VorbisState.uSampleRate);	// mSec.
}

// StartStream
AKRESULT CAkSrcFileVorbis::StartStream( )
{
	if ( m_pStream && m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < CODEBOOKHEADERDECODED )
	{
		return ProcessFirstBuffer( );
	}

	InitVorbisState( );

	m_pBuffer			= NULL;     // Buffer currently granted.
	m_pNextAddress		= NULL;     // Address of next client buffer.
	m_ulSizeLeft		= 0;        // Size left in stream buffer.

    m_ulFileOffset      = 0;

	m_bIsLastStmBuffer  = false;

    // Looping data.
    m_ulLoopStart       = 0;        // Loop start position: Byte offset from beginning of stream.
    m_ulLoopEnd         = 0;        // Loop back boundary from beginning of stream.
    m_uiCorrection      = 0;        // Correction amount (when loop start not on sector boundary).

    // Create stream.
    assert( AK::IAkStreamMgr::Get( ) );
    AKRESULT eResult;

    // Get audio context data for stream settings.

    // File name.
    AkSrcDescriptor srcDesc;
    eResult = m_pCtx->GetSrcDescriptor( &srcDesc );
    if ( eResult != AK_Success ||
         ( srcDesc.pvPath == NULL &&
           srcDesc.ulFileID == AK_INVALID_FILE_ID ) )
    {
        //assert( !"Invalid source descriptor" );
        return AK_Fail;
    }
    // Note. srcDesc.pvPath is a null-terminated string that holds the file name.

    // Audio format.
    AkAudioFormat * pFormat = m_pCtx->GetMediaFormat( );
    if ( pFormat == NULL )
    {
        assert( !"No audio format for this sound" );
        return AK_Fail;
    }

    // Looping info.
    m_uLoopCnt = m_pCtx->GetLooping();	// Current loop count. = m_pCtx->GetLooping( );
	m_uBufferLoopCnt = m_uLoopCnt;

    // Stream heuristics.
    AkAutoStmHeuristics heuristics;
    heuristics.uMinNumBuffers = 0;
    // Average throughput: taken from format (updated in ParseHeader() with real format. 
    heuristics.fThroughput = (AkReal32)pFormat->GetBlockAlign() * pFormat->uSampleRate / 1000.f; 
    // Looping: we do not know anything about looping points just yet.
    heuristics.uLoopStart = 0;
    heuristics.uLoopEnd = 0;
    // Priority.
    heuristics.priority = m_pCtx->GetPriority( );


    // Get first buffer either from bank (prefetched data) or from stream.

    // Get data from bank if stream sound is prefetched.
    // If it is, open stream paused, set position to end of prefetched data, then resume.
    bool bDoUsePrefetchedData = false;
    if ( m_pCtx->IsPrefetched() && ( m_pCtx->GetSourceOffset() == 0 ) )
	{
		m_pCtx->GetDataPtr( m_pBuffer, m_ulSizeLeft );
        bDoUsePrefetchedData = ( m_pBuffer != NULL && m_ulSizeLeft != 0 );
	}

    // Generation of complete file name from bank encoded name and global audio source path settings
    // is done at the file system level. Just fill up custom FS parameters to give it a clue.
    AkFileSystemFlags fileSystemFlags = 
    {
        AKCOMPANYID_AUDIOKINETIC,               // Vendor ID. AK uses AKVENDOR_ID_AUDIOKINETIC (defined in AkTypes.h).
        CODECID_FROM_PLUGINID( srcDesc.uiID ),  // File/Codec type ID (defined in AkTypes.h).
        NULL,									// User parameter size.
        0,										// User parameter.
        (srcDesc.bIsLanguageSpecific)			// True when file location depends on current language.
    };

    // Open.
    // Use string overload if pszFilename is set (would be set by the WAL, in !NOWWISE builds only).
    // Otherwise use ID overload.
#ifndef AK_OPTIMIZED
    if ( srcDesc.pvPath == NULL )
    {
        assert( srcDesc.ulFileID != AK_INVALID_FILE_ID );
        eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                    srcDesc.ulFileID,   // Application defined ID.
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Auto stream heuristics.
                                    NULL,               // Stream buffer constraints: none.
                                    m_pStream );        // IAkAutoStream ** out_ppStream.
    }
    else
    {
        eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                    (AkLpCtstr)srcDesc.pvPath,  // Application defined string (title only, or full path, or code...).
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Auto stream heuristics.
                                    NULL,               // Stream buffer constraints: none.
                                    m_pStream );        // IAkAutoStream ** out_ppStream.
    }
#else
    eResult = AK::IAkStreamMgr::Get( )->CreateAuto( 
                                srcDesc.ulFileID,   // Application defined string (title only, or full path, or code...).
                                &fileSystemFlags,   // File system special parameters.
                                heuristics,         // Auto stream heuristics.
                                NULL,               // Stream buffer constraints: none.
                                m_pStream );        // IAkAutoStream ** out_ppStream.

#endif

    if ( eResult != AK_Success )
    {
        assert( m_pStream == NULL );
        //assert( !"Could not open stream" ); Monitored at a lower level.
        return eResult;
    }

    assert( m_pStream != NULL );

    // In profiling mode, name the stream.
    // Profiling: Create a string out of FileID.
#ifndef AK_OPTIMIZED
    if ( srcDesc.pvPath != NULL )
	{
        m_pStream->SetStreamName( (AkLpCtstr)srcDesc.pvPath );
	}
    else
    {
        const unsigned long MAX_NUMBER_STR_SIZE = 11;
        AkTChar szName[MAX_NUMBER_STR_SIZE];
        wsprintf( szName, L"%u", srcDesc.ulFileID );
        m_pStream->SetStreamName( szName );
    }
#endif

	// do this now as the context is unknown of the codec
	AkVirtualQueueBehavior eBehavior;
	AkBelowThresholdBehavior eBelowThresholdBehavior = m_pCtx->GetVirtualBehavior( eBehavior );
	if (	(  eBelowThresholdBehavior == AkBelowThresholdBehavior_SetAsVirtualVoice 
			&& eBehavior == AkVirtualQueueBehavior_FromElapsedTime 
			) 
		|| m_pCtx->GetSourceOffset() != 0 )
	{
		m_VorbisState.bNeedSeekTable = true;
	}
	else
	{
		m_VorbisState.bNeedSeekTable = false;
	}

	// If we started the stream paused, set position and resume now.
    if ( bDoUsePrefetchedData )
    {
		// Skip prefetch.
        AkInt64 lRealPosition, lSizeMove;
        lSizeMove = m_ulSizeLeft;
        if ( m_pStream->SetPosition( lSizeMove,
                                     AK_MoveBegin,
                                     &lRealPosition ) != AK_Success )
        {
            assert( !"Could not set file position" );
            return AK_Fail;
        }

        // Parse header.
        eResult = ParseHeader( m_uLoopCnt );
		m_uBufferLoopCnt = m_uLoopCnt;
        if ( eResult != AK_Success )
        {
            return eResult;
        }
		// Use stream buffer address and consume even if we are in the bank, DecodeVorbisHeader() otherwise does not see a difference
		m_ulSizeLeft -= m_ulDataOffset;
		m_pNextAddress = m_pBuffer + m_ulDataOffset;

		LoopInit();

		// If prefetch is used, the full header and setup information should be in the bank 
		eResult = DecodeVorbisHeader(  );
		assert( eResult != AK_FormatNotReady );
		if ( eResult != AK_Success )
		{
			return eResult;
		}

		// It is possible that the stream position be set again here (if looping occured).
        // It will override previous stream position set.
		m_ulSizeLeft += m_ulDataOffset + m_VorbisState.VorbisInfo.dwVorbisDataOffset;		// return what we should not have consumed...
        m_uiCorrection = m_ulDataOffset + m_VorbisState.VorbisInfo.dwVorbisDataOffset;		// Use correction value to skip the header.
        ProcessStreamBuffer( );

	    // If we did not loop inside the prefetch data...
        if ( 0 == m_uDidLoop )
        {
        	// Now set the correction to the correct value, so that next access to StreamMgr takes the block size into account.
        	assert( ( lSizeMove - lRealPosition ) >= 0 );
        	m_uiCorrection = (AkUInt32)( lSizeMove - lRealPosition );
			m_ulFileOffset = (AkUInt32)lRealPosition;
		}

        // Start IO.
        eResult = m_pStream->Start( );
		if ( eResult != AK_Success )
		{
			return eResult;
		}

        return AK_Success;
    }

    // No prefetching.
    // Read and parse until we get the whole header.
    // NOTE. Header size is limited to a stream buffer size.

    // Start IO.
    eResult = m_pStream->Start( );
	if ( eResult != AK_Success )
	{
		return eResult;
	}
    return ProcessFirstBuffer( );
}

// StopStream
void CAkSrcFileVorbis::StopStream( )
{
	TermVorbisState();

	ReleaseBuffer();

	if ( m_VorbisState.pSeekTable )
	{
		AkFree( g_VorbisPoolId, m_VorbisState.pSeekTable );
		m_VorbisState.pSeekTable = NULL;
	}

	if ( m_pStitchStreamBuffer )
	{
		AkFree( g_VorbisPoolId, m_pStitchStreamBuffer );
		m_pStitchStreamBuffer = NULL;
		m_uStitchBufferLeft = 0;
		m_uStitchBufferEndOffset = 0;
		m_uStitchBufferValidDataSize = 0;
	}

	m_VorbisState.Allocator.Term();

	// Otherwise same as parent
	CAkSrcFileBase::StopStream( );
}

AKRESULT CAkSrcFileVorbis::StopLooping()
{
	m_uLoopCnt = 1;
	m_uBufferLoopCnt = 1;
	// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
	// so make sure that uCurrentPCMFrame will never be bigger than this one
	m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
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

// VirtualOn
void CAkSrcFileVorbis::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	// When starting virtual, stream is not created yet.
 	if ( m_pStream )
	{
		m_pStream->Stop();

		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning || eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		{
			if( m_pStitchStreamBuffer!= NULL)
			{
				AkFree( g_VorbisPoolId, m_pStitchStreamBuffer );
				m_pStitchStreamBuffer = NULL;
				m_uStitchBufferEndOffset = 0;
				m_uStitchBufferLeft = 0;
				m_uStitchBufferValidDataSize = 0;
			}
			m_VorbisState.TremorInfo.ReturnInfo.bPacketLeftOver = false;
		}

		// Nothing to do for resume mode
	}
}

// VirtualOff
AKRESULT CAkSrcFileVorbis::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
	if ( m_pStream )
	{
		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning || eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
		{
			assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );
			// Re-Initialize global decoder state
			vorbis_dsp_restart( &m_VorbisState.TremorInfo.VorbisDSPState );

			if ( m_ulSizeLeft != 0 )
			{
				m_pStream->ReleaseBuffer();
				m_pBuffer = NULL;
				m_pNextAddress = NULL;
				m_ulSizeLeft = 0;
			}

			m_bIsLastStmBuffer = false;

			if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
			{
				AKRESULT eResult = VirtualSeek( m_VorbisState.uCurrentPCMFrame );
				if( eResult != AK_Success )
				{
					return eResult;
				}
			}
			else
			{
				SeekStream( m_ulDataOffset + m_VorbisState.VorbisInfo.dwVorbisDataOffset );
				LoopInit();
			}
		}
		// Nothing to do for resume mode
		return m_pStream->Start();
	}

	return AK_Success;
}

// Time skip
AKRESULT CAkSrcFileVorbis::TimeSkip( AkUInt32 & io_uFrames )
{
	m_VorbisState.uBufferStartPCMFrame = m_VorbisState.uCurrentPCMFrame;
	m_VorbisState.uCurrentPCMFrame += io_uFrames;

	// Check for looping (and end) condition
	AkUInt32 uEndLimit = DoLoop() ? m_uPCMLoopEnd : m_VorbisState.VorbisInfo.dwTotalPCMFrames-1;
    if ( m_VorbisState.uCurrentPCMFrame >= uEndLimit )
    {
		io_uFrames -= m_VorbisState.uCurrentPCMFrame - uEndLimit; // return data 

        if ( DoLoop() )
        {
			m_VorbisState.uCurrentPCMFrame = m_uPCMLoopStart;
			if( m_uLoopCnt != LOOPING_INFINITE )
			{
				--m_uLoopCnt;
				m_uBufferLoopCnt = m_uLoopCnt;
				if ( !DoLoop() )
				{
					m_VorbisState.uCurrentPCMFrame = m_uPCMLoopStart - m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset;
					// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
					// so make sure that uCurrentPCMFrame will never be bigger than this one
					m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
				}
			}
        }
        else
        {
			// Hit the end.
			m_bIsLastStmBuffer = true;

			TimeSkipMarkers( m_VorbisState.uBufferStartPCMFrame, io_uFrames, m_VorbisState.VorbisInfo.dwTotalPCMFrames );

			return AK_NoMoreData;
        }
	}

	TimeSkipMarkers(m_VorbisState.uBufferStartPCMFrame, io_uFrames, m_VorbisState.VorbisInfo.dwTotalPCMFrames );

	return AK_DataReady;
}

// VirtualSeek
// Determine where to seek to using seek table
AKRESULT CAkSrcFileVorbis::VirtualSeek( AkUInt32 & io_uSeekPosition )
{
	// Sequentially run in seek table to determine seek position
	// TODO: Bisection algorithm would yield better performance
	AkUInt32 index = 0;
	AkUInt32 uNumSeekTableItems = m_VorbisState.VorbisInfo.dwSeekTableSize / sizeof(AkVorbisSeekTableItem);
	for ( ; index < uNumSeekTableItems; ++index )
	{
		if ( m_VorbisState.pSeekTable[index].dwPacketFirstPCMFrame > io_uSeekPosition )
		{
			break;	
		}
	}

	// if size is not null, it mean that we have a seek table and it has at least an entry
	if( uNumSeekTableItems )
	{
		if ( index > 0 )
		{
			--index;
		}

		assert( m_VorbisState.pSeekTable );
		AkInt64 lSeekPosition = m_ulDataOffset + m_VorbisState.pSeekTable[index].dwPacketFileOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		io_uSeekPosition = m_VorbisState.pSeekTable[index].dwPacketFirstPCMFrame;

		SeekStream( lSeekPosition );
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_VorbisIMRequireSeekTable );
		return AK_Fail;
	}

	return AK_Success;
}

//================================================================================
// Decoding of seek table and 3 Vorbis headers
//================================================================================
AKRESULT CAkSrcFileVorbis::DecodeVorbisHeader( )
{
	AkInt32 iResult;

	if ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets == UNINITIALIZED )
	{
		AKRESULT eResult = InitVorbisInfo();
		if(eResult != AK_Success)
		{
			return eResult;
		}

		if(!m_VorbisState.Allocator.Init(m_VorbisState.VorbisInfo.dwDecodeAllocSize))
		{
			return AK_Fail;
		}
	}

	// Try to get the setup, comment and codebook headers and set up the Vorbis decoder
	// Any error while decoding header is fatal.
	while( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < CODEBOOKHEADERDECODED ) 
	{
		// Exit if no data left
		if ( m_ulSizeLeft == 0 )
		{
			return AK_FormatNotReady;
		}

		// Read seek table
		if ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < SEEKTABLEINTIALIZED )
		{
			if ( m_VorbisState.uSeekTableSizeRead < m_VorbisState.VorbisInfo.dwSeekTableSize )
			{
				AkUInt32 uCopySize = AkMin( m_ulSizeLeft, m_VorbisState.VorbisInfo.dwSeekTableSize - m_VorbisState.uSeekTableSizeRead );
				if ( m_VorbisState.bNeedSeekTable )
				{
					// Read all seek table items
					assert( m_VorbisState.pSeekTable != NULL && m_VorbisState.VorbisInfo.dwSeekTableSize > 0 );
					AkMemCpy( (AkUInt8*)m_VorbisState.pSeekTable + m_VorbisState.uSeekTableSizeRead, m_pNextAddress, uCopySize ); 
				}
				// Just skip over it otherwise
				m_VorbisState.uSeekTableSizeRead += uCopySize;
				ConsumeData( uCopySize );
			}

			if ( m_VorbisState.uSeekTableSizeRead == m_VorbisState.VorbisInfo.dwSeekTableSize )
			{
				m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets = SEEKTABLEINTIALIZED;
			}	
		}

		// Read Vorbis header packets
		AkVorbisPacketReader PacketReader( m_pNextAddress, m_ulSizeLeft );
		while ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= SEEKTABLEINTIALIZED && m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < CODEBOOKHEADERDECODED ) 
		{
			// Get the next packet
			ogg_packet OggPacket;
			bool bLastPacket;
			bool bGotPacket = PacketReader.ReadPacket(	m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets,
														m_VorbisState.TremorInfo.bNoMoreInputPackets,
														OggPacket,
														bLastPacket );
			if ( !bGotPacket )
			{
				return AK_FormatNotReady;
			}
			else
			{
				// Otherwise write Vorbis header
				// Synthesize Vorbis header	
				iResult = vorbis_dsp_headerin(
					&m_VorbisState.TremorInfo.VorbisDSPState.vi,
					&OggPacket,
					m_VorbisState.Allocator);
				if ( iResult )
				{
					// DO NOT ASSERT! Can fail because of failed _ogg_malloc() in merge_sort. assert( !"Failure synthesizing setup header." );
					return AK_Fail;
				}
				++m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets;
			}
		}
		ConsumeData(PacketReader.GetSizeConsumed());
	}

	// Only get here once all three headers are parsed, complete Vorbis setup
	assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );

	// Initialize global decoder state
	iResult = vorbis_dsp_init(&m_VorbisState.TremorInfo.VorbisDSPState,m_VorbisState.Allocator);
	if ( iResult )
	{
		// DO NOT ASSERT! Can fail because of failed _ogg_malloc(). assert( !"Failure initializing Vorbis decoder." );
		return AK_Fail;
	}

    return AK_Success;
}

void CAkSrcFileVorbis::InitVorbisState( )
{
	// we need to keep this one as it was until the stream is stopped
#ifdef WIN32	
	bool bSSE2 = m_VorbisState.TremorInfo.VorbisDSPState.bSSE2;
#endif	
	memset(&m_VorbisState, 0, sizeof(AkVorbisSourceState));
	m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets = UNINITIALIZED;
#ifdef WIN32	
	m_VorbisState.TremorInfo.VorbisDSPState.bSSE2 = bSSE2;
#endif	
}

AKRESULT CAkSrcFileVorbis::TermVorbisState( )
{
	vorbis_dsp_clear( &m_VorbisState.TremorInfo.VorbisDSPState, m_VorbisState.Allocator);
	return AK_Success;
}

AKRESULT CAkSrcFileVorbis::InitVorbisInfo()
{
	// Initialize vorbis info
	vorbis_info_init( &m_VorbisState.TremorInfo.VorbisDSPState.vi );

	if(m_VorbisState.bNeedSeekTable == true)
	{
		if ( m_VorbisState.VorbisInfo.dwSeekTableSize == 0 )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_VorbisSourceRequireSeekTable );
			// No seek table was encoded with the file. This option is not compatible with 
			// requested virtual source behavior playback from elapsed time.
			return AK_Fail;
		}

		// Allocate seek table			
		m_VorbisState.pSeekTable = (AkVorbisSeekTableItem *) AkAlloc( g_VorbisPoolId, m_VorbisState.VorbisInfo.dwSeekTableSize );
		if ( m_VorbisState.pSeekTable == NULL )
		{
			return AK_InsufficientMemory;
		}		
	}

	m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets = INITIALIZED;
	return AK_Success;
}
//================================================================================
// ProcessStreamBuffer
// Override default implementation
//================================================================================
// REVIEW: This method duplicates CAkSrcFileBase for no reason (but beware of "m_uBufferLoopCnt").
AKRESULT CAkSrcFileVorbis::ProcessStreamBuffer( )
{
	AKASSERT( m_pStream );

	m_ulFileOffset += m_ulSizeLeft;

    // Set next pointer.
    AKASSERT( m_pBuffer != NULL );
    m_pNextAddress = m_pBuffer + m_uiCorrection;

    // Update size left.
    m_ulSizeLeft -= m_uiCorrection;

    // Will we hit the loopback boundary?
	AkUInt32 ulEndLimit = DoLoop() ? m_ulLoopEnd : m_ulDataOffset + m_ulDataSize;
    if ( m_ulFileOffset >= ulEndLimit )
    {
		// Yes. Correct size left.
		AkUInt32 ulCorrectionAmount = m_ulFileOffset - ulEndLimit;
		AKASSERT( m_ulSizeLeft >= ulCorrectionAmount ||
			!"Missed the position change at last stream read" );
		m_ulSizeLeft -= ulCorrectionAmount;

		if ( DoLoop() )
		{
			AkInt64 lRealOffset, lSizeMove;
			lSizeMove = m_ulLoopStart;
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
			// Set file offset to true value.
			m_uiCorrection = (AkUInt32)( m_ulLoopStart - (AkUInt32)lRealOffset);
			m_ulFileOffset = (AkUInt32)lRealOffset;

			// if not done and not infinite looping
			if(m_uLoopCnt > 1)
			{
				--m_uLoopCnt;
			}

			++m_uDidLoop;

	        // Update heuristics to end of file if last loop.
            if ( m_uLoopCnt == 1 ) 
			{
				// Set stream's heuristics to non-looping.
				AkAutoStmHeuristics heuristics;
				m_pStream->GetHeuristics( heuristics );
				heuristics.uLoopEnd = 0;
				m_pStream->SetHeuristics( heuristics );
			}
		}
        else
        {
			// Hit the loop release (end of file) boundary: will be NoMoreData.
			// Don't care about correction.
			// Set this flag to notify output.
			m_bIsLastStmBuffer = true;
        }
    }
    else
    {
        // If it will not loop inside that buffer, reset the offset.
        m_uiCorrection = 0;
    }
    return AK_Success;
}
//================================================================================
//================================================================================
AKRESULT CAkSrcFileVorbis::ProcessFirstBuffer( )
{
	assert( m_ulSizeLeft == 0 );
    AKRESULT eResult = m_pStream->GetBuffer(
                            (void*&)m_pBuffer,	// Address of granted data space.
                            m_ulSizeLeft,		// Size of granted data space.
                            false );

	if ( eResult == AK_NoDataReady )
    {
        // Not ready. Leave.
		m_bIsFirstBufferPending = true;
        return AK_FormatNotReady;
    }
    else if ( eResult != AK_DataReady && eResult != AK_NoMoreData )
    {
        assert( !"Could not read stream buffer" );
        return AK_Fail;
    }

    if ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets == UNINITIALIZED )
	{
		// Parse header. 
		eResult = ParseHeader( m_uLoopCnt );
		m_uBufferLoopCnt = m_uLoopCnt;
		if ( eResult != AK_Success )
		{
			return eResult;
		}

		LoopInit();

		// Set correction offset to end of header.
		m_uiCorrection = m_ulDataOffset;
	}

	// Process buffer
    eResult = ProcessStreamBuffer( );
	if ( eResult != AK_Success )
	{
		return eResult;
	}

	assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < CODEBOOKHEADERDECODED );

	// Need to setup headers first
	eResult = DecodeVorbisHeader( ); 
	if ( eResult == AK_Success )
	{
		m_bIsFirstBufferPending = false;
	}

	AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
	if( l_numNativeSampleOffset && m_VorbisState.pSeekTable )
	{
		AkUInt64 l_numRealSampleOffset = l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE;

		AkUInt32 uRealOffset = l_numRealSampleOffset;

		// Re-Initialize global decoder state
		assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );

		if ( m_ulSizeLeft != 0 )
		{
			m_pStream->ReleaseBuffer();
			m_pBuffer = NULL;
			m_pNextAddress = NULL;
			m_ulSizeLeft = 0;
		}

		eResult = VirtualSeek( uRealOffset );
		if( eResult != AK_Success )
		{
			return eResult;
		}
		
		assert( m_pStitchStreamBuffer == NULL );

		m_VorbisState.TremorInfo.ReturnInfo.bPacketLeftOver = false;

		assert( l_numRealSampleOffset - uRealOffset >= 0 );
		m_pCtx->SetSourceOffset( l_numRealSampleOffset - uRealOffset );
	}

	return eResult;
}
//================================================================================
// ParseHeader
// Parse header information
//================================================================================
AKRESULT CAkSrcFileVorbis::ParseHeader( AkInt16 in_sNumLoop )
{
   // Got the first buffer. Parse.
	WaveFormatEx fmt;
    AKRESULT eResult = CAkFileParser::Parse( m_pBuffer,			// Buffer to be parsed.
                                     m_ulSizeLeft,				// Buffer size.
                                     &fmt,						// Returned audio format.
									 sizeof( WaveFormatEx ),
                                     &m_markers,				// Markers.
                                     &m_uPCMLoopStart,			// Loop start position (in sample frames). NULL if not wanted.
									 &m_uPCMLoopEnd,			// Loop end position (in sample frames). NULL if not wanted.
									 &m_ulDataSize,				// Data size.
									 &m_ulDataOffset,			// Data offset.
									 &m_VorbisState.VorbisInfo,	// Vorbis specific information
									 sizeof( AkVorbisInfo) );	// Vorbis specific information size
    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
        return eResult;
    }

	// TMP multichannel
	m_VorbisState.TremorInfo.uChannelMask = ( fmt.nChannels == 2 ) ? AK_SPEAKER_SETUP_STEREO : AK_SPEAKER_SETUP_MONO;
	m_VorbisState.uSampleRate = fmt.nSamplesPerSec;

	// Fix loop start and loop end for no SMPL chunk
	if((m_uPCMLoopStart == 0) && (m_uPCMLoopEnd == 0))
	{	
		m_uPCMLoopEnd = m_VorbisState.VorbisInfo.dwTotalPCMFrames-1;
	}

	// Loop points. If not looping or ulLoopEnd is 0 (no loop points),
    // set loop points to data chunk boundaries.
    if ( IS_LOOPING( in_sNumLoop ) )
    {   
		// NOTE: Disregard loop points contained in Fmt chunk and use VorbisInfo instead
		m_ulLoopStart = m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartPacketOffset + m_ulDataOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		m_ulLoopEnd = m_VorbisState.VorbisInfo.LoopInfo.dwLoopEndPacketOffset + m_ulDataOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
    }
	else
	{
		m_ulLoopStart = m_ulDataOffset + m_VorbisState.VorbisInfo.dwVorbisDataOffset; // VorbisDataOffset == seek table + vorbis header
		m_ulLoopEnd = m_ulDataOffset + m_ulDataSize;
	}

	assert( m_pStream );
	// Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );
	if ( IS_LOOPING( in_sNumLoop ) )
    {   
		heuristics.uLoopStart = m_ulLoopStart;
        heuristics.uLoopEnd = m_ulLoopEnd;
	}
	else
	{
		heuristics.uLoopStart = 0;
        heuristics.uLoopEnd = 0;
	}
	heuristics.fThroughput = fmt.nAvgBytesPerSec / 1000.f;	// Throughput (bytes/ms)
    heuristics.priority = m_pCtx->GetPriority();	// Priority.
    m_pStream->SetHeuristics( heuristics );

    return AK_Success;
}
//================================================================================
//Convert Sample number to a file data offset, used for starting at non zero position.
//================================================================================
AkUInt32 CAkSrcFileVorbis::SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ )
{
	//This function is not used for Vorbis files, it uses VirtualSeek helper instead.
	out_uRoundingError = 0;
	return 0;
}

//================================================================================
// RetrieveBuffer
// Retrieve new data from input stream
//================================================================================
AKRESULT CAkSrcFileVorbis::RetrieveBuffer( )
{
	m_pStream->ReleaseBuffer( );

	// Update priority heuristic.
	AkAutoStmHeuristics heuristics;
	m_pStream->GetHeuristics( heuristics );
	heuristics.priority = m_pCtx->GetPriority( );
	m_pStream->SetHeuristics( heuristics );

	AKRESULT eStmResult;
	// No data left. Ask the stream manager.
	eStmResult = m_pStream->GetBuffer( (void*&)m_pBuffer,			// Address of granted data space.
										m_ulSizeLeft,				// Size of granted data space.
										false );					// Block until data is ready.
	if ( eStmResult == AK_NoDataReady || eStmResult == AK_Fail )
	{
		// No data ready. We don't want to block. Leave now.
		m_pNextAddress = NULL;
		return eStmResult;
	}
	else if ( m_ulSizeLeft != 0 )
	{
		if ( ProcessStreamBuffer( ) != AK_Success )
		{
			return AK_Fail;
		}
	}
	else
	{
		assert( eStmResult == AK_NoMoreData );
		if ( !m_bIsLastStmBuffer ) 
		{
			assert( !"Unexpected end of streamed audio file" );
			return AK_Fail;
		}
	}

	return eStmResult;
}
//================================================================================
// SeekStream
// Move position with the stream
//================================================================================
void CAkSrcFileVorbis::SeekStream( AkInt64 in_lSeekPosition )
{
	AkInt64 lRealOffset;
	m_pStream->SetPosition(	in_lSeekPosition,   // Seek offset.
							AK_MoveBegin,		// Seek method, from beginning, end or current file position.
							&lRealOffset        // Actual seek offset may differ from expected value when unbuffered IO.
												// In that case, floors to sector boundary. Pass NULL if don't care.
							);

	// Keep track of offset caused by unbuffered IO constraints.
	// Set file offset to true value.
	m_uiCorrection = (AkUInt32)( in_lSeekPosition - (AkUInt32)lRealOffset);
	m_ulFileOffset = (AkUInt32)lRealOffset;
}

void CAkSrcFileVorbis::LoopInit()
{
	m_VorbisState.uCurrentPCMFrame = 0;

	// Init state.
	m_uLoopCnt = m_pCtx->GetLooping();	// Current loop count.
	m_uBufferLoopCnt = m_uLoopCnt;
	m_VorbisState.uPCMStartLimitMarkers = 0; // Always consider frames for markers until it has looped once
	if ( !DoLoop() )
	{
		// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
		// so make sure that uCurrentPCMFrame will never be bigger than this one
		m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
	}
	else
	{
			m_VorbisState.uPCMEndLimitMarkers = m_uPCMLoopEnd; // Consider frames up to loop end for markers
	}
}

