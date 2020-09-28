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
#include "AkSrcFilePCM.h"
#include "AkFileParser.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"

CAkSrcFilePCM::CAkSrcFilePCM( CAkPBI * in_pCtx )
	: CAkSrcFileBase( in_pCtx )
	, m_pStitchBuffer( NULL )
	, m_uNumBytesBuffered( NULL )
	, m_uSizeToRelease( 0 )
{
}

CAkSrcFilePCM::~CAkSrcFilePCM()
{
}

void CAkSrcFilePCM::StopStream()
{
	if ( m_pStitchBuffer )
	{
		AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
		m_pStitchBuffer = NULL;
		m_uNumBytesBuffered = 0;
	}
	CAkSrcFileBase::StopStream();
}

void CAkSrcFilePCM::GetBuffer( AkVPLState & io_state )
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
                                            m_ulSizeLeft,       // Size of granted data space.
                                            false );            // Block until data is ready.
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

    // Deal with NoMoreData return flag.
	AkAudioFormat* pFormatInfo = m_pCtx->GetMediaFormat();
	AkUInt16 usBlockAlign = (AkUInt16)pFormatInfo->GetBlockAlign(); 
	// Number of whole sample frames.
	AkUInt32 uFramesLeft = m_ulSizeLeft / usBlockAlign;
    if ( m_bIsLastStmBuffer &&
         uFramesLeft <= io_state.buffer.MaxFrames() )
    {
        eResult = AK_NoMoreData;
    }

	// At that point, some data is ready, or there is no more data.
    // Maybe loop end or data chunk sizes were invalid.
    if ( m_ulSizeLeft == 0 &&
         eResult != AK_NoMoreData )
    {
        AKASSERT( !"Invalid loop back boundary. Wrong values in file header? Source failure." );
		io_state.result = AK_Fail;
        return;
    }

    // Give what the client wants, or what we have.
    AkUInt16 uMaxFrames = (AkUInt16)AkMin( io_state.buffer.MaxFrames(), uFramesLeft );
    
    // Might need to buffer streamed data if previous sample frame was split between
	// 2 streaming buffers (this should never happen in mono or stereo).
	if ( 0 == m_uNumBytesBuffered )
	{
		// Using streamed buffer directly: free any previously allocated stitch buffer.
		if ( m_pStitchBuffer )
		{
			AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
			m_pStitchBuffer = NULL;
		}

		// Set transfer object values.
		io_state.buffer.AttachInterleavedData( m_pNextAddress, uMaxFrames, uMaxFrames, pFormatInfo->GetChannelMask() );
		m_uSizeToRelease = uMaxFrames * usBlockAlign;

		// Check if data left after this frame represents less than one whole sample frame.
		// In such a case, buffer it.
		AKASSERT( m_ulSizeLeft >= m_uSizeToRelease );
		if ( ( m_ulSizeLeft - m_uSizeToRelease ) < usBlockAlign )
		{
			AKASSERT( !m_pStitchBuffer );
			m_pStitchBuffer = (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, AK_NUM_VOICE_REFILL_FRAMES * usBlockAlign );

			if ( m_pStitchBuffer )
			{			
				m_uNumBytesBuffered = (AkUInt16)(m_ulSizeLeft - m_uSizeToRelease);
				AKPLATFORM::AkMemCpy( m_pStitchBuffer, m_pNextAddress + m_uSizeToRelease, m_uNumBytesBuffered );

				// Increment m_uSizeToRelease a bit so that pointers updates in ReleaseBuffer()
				// take it into account.
				m_uSizeToRelease += m_uNumBytesBuffered;
			}
			else
			{
				// Cannot allocate stitch buffer: This error is unrecoverable. 
				io_state.result = AK_InsufficientMemory;
				return;
			}
		}
	}
	else
	{
		AKASSERT( m_pStitchBuffer );
		AKASSERT( m_uNumBytesBuffered < usBlockAlign );
		AkUInt32 uNumBytesCopied = uMaxFrames * usBlockAlign - m_uNumBytesBuffered;
		AKASSERT( m_uNumBytesBuffered + uNumBytesCopied <= (AkUInt32)AK_NUM_VOICE_REFILL_FRAMES * usBlockAlign ); 
		AKPLATFORM::AkMemCpy( m_pStitchBuffer + m_uNumBytesBuffered, m_pNextAddress, uNumBytesCopied );

		m_uSizeToRelease = (AkUInt16)uNumBytesCopied;

		m_uNumBytesBuffered = 0;

		// Set transfer object values.
		io_state.buffer.AttachInterleavedData( m_pStitchBuffer, uMaxFrames, uMaxFrames, pFormatInfo->GetChannelMask() );
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
		io_state.buffer.posInfo.uSampleRate = pFormatInfo->uSampleRate;
		io_state.buffer.posInfo.uStartPos = m_ulCurSample;
		io_state.buffer.posInfo.uFileEnd = m_ulDataSize / usBlockAlign;
	}
	if( bDidLoop )
	{
		m_ulCurSample = m_uPCMLoopStart + io_state.buffer.uValidFrames - ( m_uPCMLoopEnd - m_ulCurSample );
	}
	else
	{
		m_ulCurSample += io_state.buffer.uValidFrames;
	}
    io_state.result =  eResult;
}


void CAkSrcFilePCM::ReleaseBuffer()
{
    AKASSERT( m_pStream != NULL );
    AKASSERT( m_uSizeToRelease <= m_ulSizeLeft || !"Invalid released data size" );

	m_ulSizeLeft -= m_uSizeToRelease;
    m_pNextAddress += m_uSizeToRelease;
	m_uSizeToRelease = 0;	

	if ( m_ulSizeLeft == 0 )
    {
		// Note. When using prefetched data from banks, the first call to m_pStream->ReleaseBuffer() will
        // obviously fail. This should not have any incidence on the stream's consistency.
        m_pStream->ReleaseBuffer();
    }
	
}

void CAkSrcFilePCM::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	// Free stitch buffer if playback will be reset (that is, FromBeginning or 
	// FromElapsedTime before last streaming buffer.
	if ( m_pStitchBuffer )
	{
		if ( eBehavior == AkVirtualQueueBehavior_FromBeginning 
			|| ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime &&
				!m_bIsLastStmBuffer ) )
		{
			AkFree( g_LEngineDefaultPoolId, m_pStitchBuffer );
			m_pStitchBuffer = NULL;
			m_uNumBytesBuffered = 0;
		}
	}
	CAkSrcFileBase::VirtualOn( eBehavior );
}

AKRESULT CAkSrcFilePCM::TimeSkip( AkUInt32 & io_uFrames )
{
    if ( m_bIsLastStmBuffer )
        return CAkVPLSrcNode::TimeSkip( io_uFrames );

	AKRESULT eResult = AK_DataReady;
	AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
	AkUInt16 usBlockAlign = (AkUInt16)pFormat->GetBlockAlign(); 
	AkUInt32 uBytesRequested = io_uFrames * usBlockAlign;
	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( m_ulFileOffset - m_ulDataOffset ) / usBlockAlign;


	// if we are here, need to 'virtually seek'.

	m_ulFileOffset += uBytesRequested;

	AkUInt32 ulEndLimit = DoLoop() ? m_ulLoopEnd : m_ulDataOffset + m_ulDataSize;
	if ( m_ulFileOffset >= ulEndLimit )
    {
		io_uFrames -= ( m_ulFileOffset - ulEndLimit ) / pFormat->GetBlockAlign(); // return data 

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
		
	TimeSkipMarkers( l_ulCurrSampleOffset, io_uFrames, m_ulDataSize / usBlockAlign );
	return eResult;
}

AKRESULT CAkSrcFilePCM::ParseHeader( AkInt16 in_sNumLoop )
{
    AKASSERT( m_pStream );

    // Got the first buffer. Parse.
	
	WaveFormatEx fmt;

    AKRESULT eResult = CAkFileParser::Parse(	m_pBuffer,		// Buffer to be parsed.
												m_ulSizeLeft,   // Buffer size.
												&fmt,			// Returned audio format.
									 			sizeof( WaveFormatEx ),
												&m_markers,		// Markers.
												&m_uPCMLoopStart,   // Loop start position (in sample frames). NULL if not wanted.
												&m_uPCMLoopEnd,		// Loop end position (in sample frames). NULL if not wanted.
												&m_ulDataSize,	// Data size.
												&m_ulDataOffset,// Data offset.
												NULL,			// No additional format info for PCM 	
												NULL );			// Size of additional format info
    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
        return eResult;
    }
	
	AKASSERT( m_ulDataOffset % 4 == 0 );

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
        m_ulLoopEnd    = ulEndOfData;
    }
    else
    {	
		// If LoopEnd is non zero, then it is in sample frames and the loop should include the end sample.
        // Convert to bytes offset, from beginning of FILE.
		AkUInt32 uiBlockAlign = fmt.nBlockAlign;

		m_ulLoopStart = m_ulDataOffset + uiBlockAlign*m_uPCMLoopStart;
		m_ulLoopEnd  = m_ulDataOffset + uiBlockAlign*(m_uPCMLoopEnd+1);

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
    heuristics.fThroughput = (AkReal32) ( fmt.nBlockAlign * fmt.nSamplesPerSec ) / 1000.f;

    // Looping.
    if ( IS_LOOPING( in_sNumLoop ) )
    {
        heuristics.uLoopStart = m_ulLoopStart;
        heuristics.uLoopEnd = m_ulLoopEnd;
    }

    // Priority.
    heuristics.priority = m_pCtx->GetPriority();

    m_pStream->SetHeuristics( heuristics );
    
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: SampleOffsetToFileOffset()
// Desc: Transform number of sample offset to file offset.
//
// Return: AkUInt32 : File Offset.
//
//-----------------------------------------------------------------------------
AkUInt32 CAkSrcFilePCM::SampleOffsetToFileOffset( AkUInt32 in_uNumSamples, AkUInt32& out_uRoundingError/*num samples*/ )
{
	out_uRoundingError = 0;
	return in_uNumSamples * m_pCtx->GetMediaFormat()->GetBlockAlign();
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkSrcFilePCM::GetDuration( void ) const
{	 
	//Maths must be done in float, 32bits will overflow too easily.
	AkAudioFormat * l_pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( l_pFormat != NULL );
    AKASSERT( m_ulLoopEnd >= m_ulLoopStart );

    AkUInt16 uNumLoops = m_pCtx->GetLooping( );
    if ( uNumLoops == 0 )
        return 0;

    AkReal32 l_fTotalSize = (AkReal32)m_ulDataSize + (AkReal32)(uNumLoops-1)*(m_ulLoopEnd-m_ulLoopStart);
	AkReal32 l_fSamples = l_fTotalSize / (AkReal32)l_pFormat->GetBlockAlign();
	AkReal32 l_fRate    = (AkReal32)(l_pFormat->uSampleRate);

    return (AkTimeMs)((l_fSamples*1000.f)/l_fRate);		// mSec.
}
