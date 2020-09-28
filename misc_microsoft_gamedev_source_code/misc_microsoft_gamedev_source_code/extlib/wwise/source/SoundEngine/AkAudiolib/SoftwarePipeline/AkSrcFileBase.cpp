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

#include "StdAfx.h"
#include "AkSrcFileBase.h"
#include "AkLEngine.h"
#include "AudiolibDefs.h"
#include "AkFileParser.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"

CAkSrcFileBase::CAkSrcFileBase( CAkPBI * in_pCtx )
	: CAkSrcBaseEx( in_pCtx )
	, m_pStream( NULL )
	, m_ulDataSize( 0 )
	, m_ulDataOffset( 0 )
    , m_pBuffer( NULL )
    , m_pNextAddress( NULL )
    , m_ulSizeLeft( 0 )
    , m_bIsLastStmBuffer( false )
    , m_bIsFirstBufferPending( false )
    , m_ulFileOffset( 0 )
	, m_ulCurSample( 0 )
	, m_ulLoopStart( 0 )
	, m_ulLoopEnd( 0 )
	, m_uiCorrection( 0 )
	, m_uDidLoop( 0 )
{
}

CAkSrcFileBase::~CAkSrcFileBase()
{
    if ( m_pStream != NULL )
    {
		AKASSERT( !"This should not happen" );
		StopStream();
    }
}

AKRESULT CAkSrcFileBase::StartStream()
{
	AKASSERT( m_pStream == NULL || m_bIsFirstBufferPending );

    // Try process first buffer if stream is already created and running
    if ( m_bIsFirstBufferPending )
    {
        AKASSERT( m_pStream != NULL );
        return ProcessFirstBuffer( );
    }

    // Create stream.
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AKRESULT eResult;

    // Get audio context data for stream settings.

    // File name.
    AkSrcDescriptor srcDesc;
    eResult = m_pCtx->GetSrcDescriptor( &srcDesc );
    if ( eResult != AK_Success ||
         ( srcDesc.pvPath == NULL &&
           srcDesc.ulFileID == AK_INVALID_FILE_ID ) )
    {
        //AKASSERT( !"Invalid source descriptor" );
        return AK_Fail;
    }
    // Note. srcDesc.pvPath is a null-terminated string that holds the file name.

    // Audio format.
    AkAudioFormat * pFormat = m_pCtx->GetMediaFormat( );
	AKASSERT( pFormat );

    // Looping info.
    AkInt16 sNumLoop = m_pCtx->GetLooping( );

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
        AKASSERT( m_pStream == NULL );
        //AKASSERT( !"Could not open stream" ); Monitored at a lower level.
        return eResult;
    }

    AKASSERT( m_pStream != NULL );

    // In profiling mode, name the stream.
    // Profiling: create a string out of FileID.
#ifndef AK_OPTIMIZED
    if ( srcDesc.pvPath != NULL )
        m_pStream->SetStreamName( (AkLpCtstr)srcDesc.pvPath );
    else
    {
        const unsigned long MAX_NUMBER_STR_SIZE = 11;
        AkTChar szName[MAX_NUMBER_STR_SIZE];
#if defined(WIN32) || defined(XBOX360)
        wsprintf( szName, L"%u", srcDesc.ulFileID );
#else
		swprintf(szName, MAX_NUMBER_STR_SIZE, L"%u", srcDesc.ulFileID);
#endif
        m_pStream->SetStreamName( szName );
    }
#endif

    // If we started the stream paused, set position and resume now.
    if ( bDoUsePrefetchedData )
    {
        // Parse header.
        eResult = ParseHeader( sNumLoop );
        if ( eResult != AK_Success )
        {
            return eResult;
        }

        // Skip prefetch.
        AkInt64 lRealPosition, lSizeMove;
        lSizeMove = m_ulSizeLeft;
        if ( m_pStream->SetPosition( lSizeMove,
                                     AK_MoveBegin,
                                     &lRealPosition ) != AK_Success )
        {
            AKASSERT( !"Could not set file position" );
            return AK_Fail;
        }

        
        // It is possible that the stream position be set again here (if looping occured).
        // It will override previous stream position set.
        m_uiCorrection = m_ulDataOffset;    // Use correction value to skip the header.
        ProcessStreamBuffer();
	        
	    // If we did not loop inside the prefetch data...
        if ( 0 == m_uDidLoop )
        {
        	// Now set the correction to the correct value, so that next access to StreamMgr takes the block size into account.
        	AKASSERT( ( lSizeMove - lRealPosition ) >= 0 );
        	m_uiCorrection = (AkUInt32)( lSizeMove - lRealPosition );
			m_ulFileOffset = (AkUInt32)lRealPosition;
		}

        // Start IO.
        m_pStream->Start( );

        return AK_Success;
    }

    // No prefetching.
    // Read and parse until we get the whole header.
    // NOTE. Header size is limited to a stream buffer size.
    
    // Start IO.
    m_pStream->Start( );
    return ProcessFirstBuffer( );
}

void CAkSrcFileBase::StopStream()
{
    // Close stream.
    if ( m_pStream != NULL )
    {
        m_pStream->Destroy( );
        m_pStream = NULL;
    }

    m_bIsFirstBufferPending = false;

	CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcFileBase::StopLooping()
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

// Process newly acquired buffer from stream: 
// Update offset in file, 
// Reset pointers for client GetBuffer(),
// Deal with looping: update count, file position, SetStreamPosition() to loop start.
// Sets bLastStmBuffer flag.
AKRESULT CAkSrcFileBase::ProcessStreamBuffer( void )
{
    AKASSERT( m_pStream );

    // Update offset in file.
    //AKASSERT( m_ulFileOffset == m_pStream->GetPosition( NULL ) ); Not true at first call with prefetched data.

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
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;

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
        else if ( m_uLoopCnt == 1 )
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

// Process first buffer from stream: 
// If not ready, returns AK_FormatNotReady.
// Otherwise, 
// - calls parse header;
// - sets m_bIsReady flag.
AKRESULT CAkSrcFileBase::ProcessFirstBuffer( void )
{
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
    else if ( eResult != AK_DataReady &&
              eResult != AK_NoMoreData )
    {
        AKASSERT( !"Could not read stream buffer" );
        return AK_Fail;
    }

    m_bIsFirstBufferPending = false;

    // Parse header. 
    eResult = ParseHeader( m_pCtx->GetLooping( ) );
    if ( eResult != AK_Success )
    {
        return eResult;
    }

	AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
	if( l_numNativeSampleOffset )
	{
		AkInt64 lRealPosition, lSizeMove;
		AkUInt32 l_numRealSampleOffset = (AkUInt32)( l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE );
		
		AkUInt32 l_numSampleRoundingError = 0;
		lSizeMove = m_ulDataOffset + SampleOffsetToFileOffset( l_numRealSampleOffset, l_numSampleRoundingError );
		m_pCtx->SetSourceOffset( l_numSampleRoundingError );

		AKASSERT( lSizeMove < m_ulDataOffset + m_ulDataSize );

		//The offset requested is within the first streamed buffer, no need to reposition the hard stream.
		if( lSizeMove < m_ulSizeLeft )
		{
			// Set correction offset to end of header.
			m_uiCorrection = (AkUInt32)lSizeMove;
			// Process buffer, to handle case where looping would occur now.
			ProcessStreamBuffer( ); 
		}
		else
		{
			if ( m_pStream->SetPosition( lSizeMove,
										AK_MoveBegin,
										&lRealPosition ) != AK_Success )
			{
				AKASSERT( !"Could not set file position" );
				return AK_Fail;
			}

			// Keep track of offset caused by unbuffered IO constraints.
			// Set file offset to true value.
			m_uiCorrection = (AkUInt32)( lSizeMove - (AkUInt32)lRealPosition );
			m_ulFileOffset = (AkUInt32)lRealPosition;

			m_pStream->ReleaseBuffer();
			m_ulSizeLeft = 0;
		}
	}
	else
	{
		// Set correction offset to end of header.
		m_uiCorrection = m_ulDataOffset;
		// Process buffer, to handle case where looping would occur now.
		ProcessStreamBuffer( );  
	}

    return AK_Success;
}

void CAkSrcFileBase::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if( m_pStream )
	{
		m_pStream->Stop();

		if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime &&
			!m_bIsLastStmBuffer ) // Do not release/seek when last buffer.
		{
			if ( m_ulSizeLeft != 0 )
			{
				m_pStream->ReleaseBuffer();

				if ( m_ulFileOffset >= m_ulSizeLeft )
				{
					m_ulFileOffset -= m_ulSizeLeft; // in elapsed mode, m_ulFileOffset becomes our virtual file pointer
					// Ensure it lands on a sample frame boundary.
					m_ulFileOffset -= ( m_ulFileOffset % ( m_pCtx->GetMediaFormat()->GetBlockAlign() ) );
				}
				else
				{
					// If file offset is greater than size left, it means it was set to the beginning of the loop.
					// Rewind the loop count and place the file offset at the appropriate place.
					AKASSERT( m_ulLoopEnd >= m_ulSizeLeft );
					m_ulFileOffset = m_ulLoopEnd - m_ulSizeLeft;
					// It should be on a sample frame boundary.
					AKASSERT( m_ulFileOffset % ( m_pCtx->GetMediaFormat()->GetBlockAlign() ) == 0 );
				}

				m_pBuffer = NULL;
				m_pNextAddress = NULL;
				m_ulSizeLeft = 0;
			}
		}
		else if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
		{
			if ( m_ulSizeLeft != 0 )
			{
				m_pStream->ReleaseBuffer();

				m_pBuffer = NULL;
				m_pNextAddress = NULL;
				m_ulSizeLeft = 0;
			}

			AkInt64 lRealOffset;
			// Set stream position now for next read.
			AKVERIFY( m_pStream->SetPosition(
						m_ulDataOffset,     // Seek offset.
						AK_MoveBegin,       // Seek method, from beginning, end or current file position.
						&lRealOffset        // Actual seek offset may differ from expected value when unbuffered IO.
											// In that case, floors to sector boundary. Pass NULL if don't care.
						) == AK_Success );

			// Keep track of offset caused by unbuffered IO constraints.
			// Set file offset to true value.
			m_uiCorrection = (AkUInt32)( m_ulDataOffset - (AkUInt32)lRealOffset);
			m_ulFileOffset = (AkUInt32)lRealOffset;

			m_uLoopCnt = m_pCtx->GetLooping();
			m_bIsLastStmBuffer = false;
		}
	}
}

AKRESULT CAkSrcFileBase::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime &&
		!m_bIsLastStmBuffer )  // Do not release/seek when last buffer.
	{
		AkInt64 lRealOffset;
		// Set stream position now for next read.
		AKRESULT eResult = m_pStream->SetPosition(
										m_ulFileOffset,     // Seek offset.
										AK_MoveBegin,       // Seek method, from beginning, end or current file position.
										&lRealOffset );     // Actual seek offset may differ from expected value when unbuffered IO.
															// In that case, floors to sector boundary. Pass NULL if don't care.
		if( eResult != AK_Success )
			return eResult;

		// Keep track of offset caused by unbuffered IO constraints.
		// Set file offset to true value.
		m_uiCorrection = (AkUInt32)( m_ulFileOffset - (AkUInt32)lRealOffset);
		m_ulFileOffset = (AkUInt32)lRealOffset;
	}

	return m_pStream->Start();
}
