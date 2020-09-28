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

#include "StdAfx.h"
#include "AkSrcFileADPCM.h"
#include "AkLEngine.h"
#include "AkFileParser.h"
#include "AkMonitor.h"

CAkSrcFileADPCM::CAkSrcFileADPCM( CAkPBI * in_pCtx )
	: CAkSrcFileBase( in_pCtx )
	, m_wExtraSize( 0 )
	, m_pOutBuffer( NULL )
{
}

CAkSrcFileADPCM::~CAkSrcFileADPCM()
{
	ReleaseBuffer();
}

void CAkSrcFileADPCM::GetBuffer( AkVPLState & io_state )
{
    AKASSERT( m_pStream != NULL );

    AKRESULT eResult = AK_DataReady;

    // See if we need to get more data from stream manager.
    if ( m_ulSizeLeft == 0 &&
         !m_bIsLastStmBuffer )
    {
    	// Update priority heuristic.
    	AkAutoStmHeuristics heuristics;
	    m_pStream->GetHeuristics( heuristics );
	    heuristics.priority = m_pCtx->GetPriority( );
	    m_pStream->SetHeuristics( heuristics );
	    
        AKRESULT eStmResult;
        // No data left. Ask the stream manager.
        eStmResult = m_pStream->GetBuffer( (void*&)m_pBuffer,   // Address of granted data space.
                                        m_ulSizeLeft,           // Size of granted data space.
                                        false );                // Block until data is ready.
        if ( eStmResult == AK_NoDataReady )
        {
            // No data ready. We don't want to block. Leave now.
			//AKASSERT( !"No data ready." );
			io_state.result = eStmResult;
            return;
        }
        else if ( eStmResult == AK_Fail )
        {
			io_state.result = eStmResult;
            return;
        }
        else if ( m_ulSizeLeft != 0 )
        {         
            if ( ProcessStreamBuffer( ) != AK_Success )
            {
				io_state.result = AK_Fail;
				return;
            }
        }
        else 
        {
        	AKASSERT( eStmResult == AK_NoMoreData );
        	if ( !m_bIsLastStmBuffer ) 
        	{
        		AKASSERT( !"Unexpected end of streamed audio file" );
				io_state.result = AK_Fail;
				return;
        	}
        }
    }

	AkAudioFormat* pFormat = m_pCtx->GetMediaFormat();
	AkUInt32 uBlockAlign = pFormat->GetBlockAlign();
	AkChannelMask uChannelMask = pFormat->GetChannelMask();
	AkUInt32 uNumChannels = GetNumChannels( uChannelMask );

	// Deal with NoMoreData return flag.

    if ( m_bIsLastStmBuffer &&
		( m_ulSizeLeft / m_wBlockAlign * ADPCM_SAMPLES_PER_BLOCK ) <= io_state.buffer.MaxFrames() )
    {
        eResult = AK_NoMoreData;
    }

    // At that point, some data is ready, or there is no more data.
    // Maybe loop end or data chunk sizes were invalid.
    if ( m_ulSizeLeft == 0 )
	{
		AKASSERT( !"No more data: GetBuffer should not have been called" ); // Invalid loop back boundary. Wrong values in file header? Source failure.
		io_state.buffer.uValidFrames = 0;
		io_state.result = AK_Fail;
		return;
	}

	// Allocate output buffer for decoded data

	AKASSERT( !m_pOutBuffer );
	AKASSERT( io_state.buffer.MaxFrames() >= ADPCM_SAMPLES_PER_BLOCK );
	AkUInt16 uMaxFrames = AK_NUM_VOICE_REFILL_FRAMES;
	m_pOutBuffer = (AkUInt8 *)CAkLEngine::GetCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * uBlockAlign ); 
	if ( !m_pOutBuffer )
	{
		io_state.result = AK_InsufficientMemory;
        return;
	}

	AkUInt32 ulPCMBlockAlign = uBlockAlign * ADPCM_SAMPLES_PER_BLOCK;

	AkUInt8 * pOutBuffer = m_pOutBuffer;

	// Might need to process one compressed block split between input buffers

	if ( m_wExtraSize )
	{
		AKPLATFORM::AkMemCpy( m_ExtraBlock + m_wExtraSize, m_pNextAddress, m_wBlockAlign - m_wExtraSize );

		for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
			CAkADPCMCodec::Decode( m_ExtraBlock + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), 1, m_wBlockAlign, uNumChannels );
			
		m_ulSizeLeft -= ( m_wBlockAlign - m_wExtraSize );
		m_pNextAddress += ( m_wBlockAlign - m_wExtraSize );

		m_wExtraSize = 0;

		pOutBuffer += ulPCMBlockAlign;
		uMaxFrames -= ADPCM_SAMPLES_PER_BLOCK;
	}

	// Now convert this input buffer
	AkUInt32 uSizeLeftFrames = m_ulSizeLeft / m_wBlockAlign;
	AkUInt32 uNumBlocks = uMaxFrames / ADPCM_SAMPLES_PER_BLOCK;
	AkUInt32 ulADPCMBlocksToProcess = AkMin( uSizeLeftFrames, uNumBlocks);

	for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
		CAkADPCMCodec::Decode( m_pNextAddress + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), ulADPCMBlocksToProcess, m_wBlockAlign, uNumChannels );

	pOutBuffer += ulADPCMBlocksToProcess * ulPCMBlockAlign;

	uMaxFrames = (AkUInt16)(( pOutBuffer - m_pOutBuffer ) / uBlockAlign);

    // Set transfer object values.
    io_state.buffer.AttachInterleavedData( m_pOutBuffer, uMaxFrames, uMaxFrames, uChannelMask );

    m_ulSizeLeft -= ulADPCMBlocksToProcess * m_wBlockAlign;
    m_pNextAddress += ulADPCMBlocksToProcess * m_wBlockAlign;

    if ( io_state.buffer.uValidFrames == 0 )
        eResult = AK_NoDataReady;

	// If there is less than a whole compressed block left in the input buffer, 
	// keep what's left and request the next one, as we cannot process this bit by itself.

    if ( m_ulSizeLeft < m_wBlockAlign )
    {
		AKASSERT( m_ulSizeLeft < ADPCM_MAX_BLOCK_ALIGN );

		m_wExtraSize = (AkUInt16) m_ulSizeLeft;
		AKPLATFORM::AkMemCpy( m_ExtraBlock, m_pNextAddress, m_wExtraSize );

		m_pNextAddress += m_wExtraSize;
		m_ulSizeLeft = 0;

        m_pStream->ReleaseBuffer();
    }

	bool bDidLoop = false;
	if( m_uDidLoop )
	{
		if( m_ulCurSample + io_state.buffer.uValidFrames >= m_uPCMLoopEnd )
		{
			bDidLoop = true;
			--m_uDidLoop;
		}
	}
	CopyRelevantMarkers( io_state.buffer, m_ulCurSample, bDidLoop );

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = pFormat->uSampleRate;
		io_state.buffer.posInfo.uStartPos = m_ulCurSample;
		AKASSERT( (m_ulDataSize % m_wBlockAlign) == 0 );
		io_state.buffer.posInfo.uFileEnd = (m_ulDataSize / m_wBlockAlign) * ADPCM_SAMPLES_PER_BLOCK;
	}

	if( bDidLoop )
	{
		m_ulCurSample = m_uPCMLoopStart + io_state.buffer.uValidFrames - ( m_uPCMLoopEnd - m_ulCurSample );
	}
	else
	{
		m_ulCurSample += io_state.buffer.uValidFrames;
	}

	io_state.result = eResult;
}

void CAkSrcFileADPCM::StopStream()
{
	ReleaseBuffer();
	CAkSrcFileBase::StopStream();
}


void CAkSrcFileADPCM::ReleaseBuffer()
{
	if ( m_pOutBuffer )
	{
		AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
		AkUInt32 uBlockAlign = pFormat->GetBlockAlign();
		CAkLEngine::ReleaseCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * uBlockAlign, m_pOutBuffer );
		m_pOutBuffer = NULL;
	}
}

AKRESULT CAkSrcFileADPCM::TimeSkip( AkUInt32 & io_uFrames )
{
    if ( m_bIsLastStmBuffer )
        return CAkVPLSrcNode::TimeSkip( io_uFrames );

	AKRESULT eResult = AK_DataReady;
	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( m_ulFileOffset - m_ulDataOffset ) * ADPCM_SAMPLES_PER_BLOCK / m_wBlockAlign;

	AkUInt32 uBlocksRequested = io_uFrames / ADPCM_SAMPLES_PER_BLOCK;
	io_uFrames = uBlocksRequested * ADPCM_SAMPLES_PER_BLOCK;

	// if we are here, need to 'virtually seek'.

    AKASSERT( m_ulFileOffset < m_ulLoopEnd );
	m_ulFileOffset += uBlocksRequested * m_wBlockAlign;

    if ( m_ulFileOffset >= m_ulLoopEnd )
    {
		io_uFrames -= ( m_ulFileOffset - m_ulLoopEnd ) / m_wBlockAlign * ADPCM_SAMPLES_PER_BLOCK; // return data 

        if ( DoLoop() )
        {
			m_ulFileOffset = m_ulLoopStart;
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
			// Update heuristics to end of file if last loop.
            if ( m_uLoopCnt == 1 && m_pStream ) 
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
			// Hit the end.
            m_bIsLastStmBuffer = true;
			eResult = AK_NoMoreData;
        }
	}
		
	TimeSkipMarkers( l_ulCurrSampleOffset, io_uFrames, m_ulDataSize * ADPCM_SAMPLES_PER_BLOCK / m_wBlockAlign );
	return eResult;
}

void CAkSrcFileADPCM::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	CAkSrcFileBase::VirtualOn( eBehavior );

	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		m_wExtraSize = 0;
	}
}

AKRESULT CAkSrcFileADPCM::ParseHeader( AkInt16 in_sNumLoop )
{
    AKASSERT( m_pStream );

    // Got the first buffer. Parse.
	WaveFormatEx fmt;

    AKRESULT eResult = CAkFileParser::Parse(	m_pBuffer,      // Buffer to be parsed.
												m_ulSizeLeft,   // Buffer size.
												&fmt,			// Returned audio format.
												sizeof( WaveFormatEx ),
												&m_markers,		// Markers.
												&m_uPCMLoopStart,   // Loop start position (in sample frames). NULL if not wanted.
												&m_uPCMLoopEnd,		// Loop end position (in sample frames). NULL if not wanted.
												&m_ulDataSize,	// Data size.
												&m_ulDataOffset,// Data offset.
												NULL,			// No additional format info for ADPCM
												NULL);			// Size of additional info
    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
        return eResult;
    }
    
    AKASSERT( CAkADPCMCodec::IsValidImaAdpcmFormat( fmt ) );
    // Set loop points.
    AkUInt32 ulEndOfData = m_ulDataOffset + m_ulDataSize;

    // Loop points. If not looping or ulLoopEnd is 0 (no loop points),
    // set loop points to data chunk boundaries.
    if ( in_sNumLoop == 1 ||
         0 == m_uPCMLoopEnd )
    {
        // Loop start = start of data.
        m_ulLoopStart   = m_ulDataOffset;
        // Loop end = end of data.
        m_ulLoopEnd    = ulEndOfData;;
    }
    else
    {	
		// If LoopEnd is non zero, then it is in sample frames and the loop should include the end sample.
        // Convert to bytes offset, from beginning of FILE.
		AkUInt32 uiBlockAlign = fmt.nBlockAlign;

		// Find the block number that the loop points corresponds to.
		AKASSERT( (m_uPCMLoopStart % ADPCM_SAMPLES_PER_BLOCK ) == 0 ); 
		AKASSERT( ((m_uPCMLoopEnd + 1) % ADPCM_SAMPLES_PER_BLOCK ) == 0 ); 
		m_ulLoopStart   = m_ulDataOffset + uiBlockAlign * (m_uPCMLoopStart / ADPCM_SAMPLES_PER_BLOCK );
		m_ulLoopEnd    = m_ulDataOffset + uiBlockAlign * ((m_uPCMLoopEnd+1) / ADPCM_SAMPLES_PER_BLOCK );

        // Verify loop. Invalid if loop start or loop end passed the last sample of the file, or if loop end before loop start.
        if ( m_uPCMLoopEnd < m_uPCMLoopStart ||
			    m_ulLoopStart > ulEndOfData || 
                m_ulLoopEnd > ulEndOfData )
        {
        	MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
            return AK_InvalidFile;
        }
    }

    // Update stream heuristics.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );

    // Throughput.
    heuristics.fThroughput = (AkReal32) fmt.nSamplesPerSec * fmt.nBlockAlign / ( 1000.f * ADPCM_SAMPLES_PER_BLOCK );

    // Looping.
    if ( IS_LOOPING( in_sNumLoop ) )
    {
        heuristics.uLoopStart = m_ulLoopStart;
        heuristics.uLoopEnd = m_ulLoopEnd;
    }

    // Priority.
    heuristics.priority = m_pCtx->GetPriority();

    m_pStream->SetHeuristics( heuristics );

	m_wBlockAlign = fmt.nBlockAlign;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: SampleOffsetToFileOffset()
// Desc: Transform number of sample offset to file offset.
//
// Return: AkUInt32 : File Offset.
//
//-----------------------------------------------------------------------------
AkUInt32 CAkSrcFileADPCM::SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ )
{
	AkUInt32 l_numADPCMBlock = in_uNumSamples / ADPCM_SAMPLES_PER_BLOCK;
	out_uRoundingError = (AkUInt32)( in_uNumSamples - ( l_numADPCMBlock * ADPCM_SAMPLES_PER_BLOCK ) );
	return ( l_numADPCMBlock )* m_wBlockAlign;
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkSrcFileADPCM::GetDuration( void ) const
{	 
	AkAudioFormat * l_pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( l_pFormat != NULL );
    AKASSERT( m_ulLoopEnd >= m_ulLoopStart );

    AkUInt16 uNumLoops = m_pCtx->GetLooping( );
    if ( uNumLoops == 0 )
        return 0;

    AkReal32 l_fTotalSize = (AkReal32)m_ulDataSize + (AkReal32)(uNumLoops-1)*(m_ulLoopEnd-m_ulLoopStart);
	AkReal32 l_fSamples = (l_fTotalSize/(AkReal32)m_wBlockAlign) * ADPCM_SAMPLES_PER_BLOCK;
	AkReal32 l_fRate    = (AkReal32)l_pFormat->uSampleRate;

	return (AkTimeMs)((l_fSamples*1000.f)/l_fRate);		// mSec.
}



