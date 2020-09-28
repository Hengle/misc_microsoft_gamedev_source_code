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
#include "AkSrcBankADPCM.h"
#include "AkLEngine.h"
#include "AkFileParser.h"
#include "AkMonitor.h"

//-----------------------------------------------------------------------------
// Name: CAkSrcBankADPCM
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankADPCM::CAkSrcBankADPCM( CAkPBI * in_pCtx )
: CAkSrcBaseEx( in_pCtx )
, m_pOutBuffer( NULL )
, m_pucData( NULL )
, m_pucLoopStart( NULL )
, m_pucLoopEnd( NULL )
, m_ulDataSize( 0 )
, m_pucDataStart( NULL )
{
}

//-----------------------------------------------------------------------------
// Name: ~CAkSrcBankADPCM
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankADPCM::~CAkSrcBankADPCM()
{
	// ReleaseBuffer might not have been called
	ReleaseBuffer();
}

//-----------------------------------------------------------------------------
// Name: StartStream
// Desc: Start to stream data.
//
// Return: Ak_Success:          Stream was started.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to start streaming data.
//-----------------------------------------------------------------------------
AKRESULT CAkSrcBankADPCM::StartStream()
{
    AKASSERT( m_pucDataStart == NULL );

	AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
    if ( pvBuffer == NULL )
		return AK_Fail;

    AkUInt32 ulLoopStart, ulLoopEnd, uDataOffset;
    AKRESULT eResult;

	WaveFormatEx fmt;
	
	eResult = CAkFileParser::Parse( pvBuffer,   // Data buffer
                            ulBufferSize,		// Buffer size
                            &fmt,        // Returned audio format.
							sizeof( WaveFormatEx ),
                            &m_markers,         // Markers.
                            &ulLoopStart,       // Beginning of loop offset.
                            &ulLoopEnd,			// End of loop offset.
							&m_ulDataSize,		// Data size.
							&uDataOffset,	// Offset to data.
							NULL,
							NULL );
							
	if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_InvalidFile;
    }

	AKASSERT( CAkADPCMCodec::IsValidImaAdpcmFormat( fmt ) );

    m_pucDataStart = pvBuffer + uDataOffset;
	m_usBlockAlign = fmt.nBlockAlign;

	// Parsed loop start and end are relative to start of data chunk, in sample frames. 
	// Set to absolute start and end in bytes.
	m_pucLoopStart = m_pucDataStart + m_usBlockAlign * ( ulLoopStart / ADPCM_SAMPLES_PER_BLOCK );

	// Set Loop point to end of file if not looping sound or if there are no loop points.
	// Note. If there are no loop points, LoopEnd is set to 0. 
	if ( ulLoopEnd == 0 || m_pCtx->GetLooping() == 1 )
	{
		// No loop point. 
		m_pucLoopEnd = m_pucDataStart + m_ulDataSize;
	}
	else
	{
		// Loop points: translate. (Loop points inclusive)
		m_pucLoopEnd = m_pucDataStart + m_usBlockAlign * ((ulLoopEnd+1) / ADPCM_SAMPLES_PER_BLOCK );
	}
	
	// At this point we trick the loop points to match the error converted loop points in sample
	// Read : we intentionnally introduce an error.
	// If the same error is not applied to both, inconsistencies will result
	m_uPCMLoopStart = (AkUInt32) ( m_pucLoopStart - m_pucDataStart ) * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign;
	m_uPCMLoopEnd = (AkUInt32) ( m_pucLoopEnd - m_pucDataStart ) * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign;

	// Verify data buffer consistency.
	if ( m_pucLoopEnd < m_pucLoopStart ||
		m_pucLoopStart > (m_pucDataStart + m_ulDataSize) || 
		m_pucLoopEnd > (m_pucDataStart + m_ulDataSize)  || 
		( ulBufferSize != (uDataOffset + m_ulDataSize) ) )
	{
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		eResult = AK_Fail;
	}

    // Init state.
	AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
	AkUInt64 l_numRealSampleOffset = l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE;

	AkUInt32 l_numADPCMBlock = (AkUInt32)( l_numRealSampleOffset / ADPCM_SAMPLES_PER_BLOCK );
	AkUInt32 l_numSampleRoundingError = (AkUInt32)( l_numRealSampleOffset - ( l_numADPCMBlock * ADPCM_SAMPLES_PER_BLOCK ) );
	m_pCtx->SetSourceOffset( l_numSampleRoundingError );

    m_pucData = m_pucDataStart + (AkUInt32)( l_numADPCMBlock * m_usBlockAlign );// Beginning of data + Initial Offset
	AKASSERT( m_pucData < m_pucDataStart + m_ulDataSize );
	
	m_uLoopCnt = m_pCtx->GetLooping();              // Current loop count.

	return eResult;
}

//-----------------------------------------------------------------------------
// Name: StopStream
// Desc: Stop streaming data.
//
// Return: Ak_Success:          Stream was stopped.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to stop the stream.
//-----------------------------------------------------------------------------
void CAkSrcBankADPCM::StopStream()
{
	m_pucDataStart = NULL;
	ReleaseBuffer();
	CAkSrcBaseEx::StopStream();
}

void CAkSrcBankADPCM::ReleaseBuffer()
{
	if ( m_pOutBuffer )
	{
		AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
		AkUInt32 uPCMBlockAlign = GetNumChannels( pFormat->GetChannelMask() ) * sizeof(AkInt16);
		CAkLEngine::ReleaseCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * uPCMBlockAlign, m_pOutBuffer );
		m_pOutBuffer = NULL;
	}
}

void CAkSrcBankADPCM::GetBuffer( AkVPLState & io_state )
{	
	AkAudioFormat * l_pFormat = m_pCtx->GetMediaFormat();
	AkChannelMask uChannelMask = l_pFormat->GetChannelMask();
	
	AkUInt32 uNumChannels = GetNumChannels( uChannelMask );
	
	AkUInt32 ulPCMBlockAlign = uNumChannels * sizeof(AkInt16);

	// Allocate output buffer for decoded data
	AKASSERT( !m_pOutBuffer );
	m_pOutBuffer = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( AK_NUM_VOICE_REFILL_FRAMES * ulPCMBlockAlign ); 
	if ( !m_pOutBuffer )
	{
		io_state.result = AK_InsufficientMemory;
        return;
	}

	AKRESULT l_eResult = AK_DataReady;
	AkUInt8 * pOutBuffer = m_pOutBuffer;
	AkUInt8 * pucData = m_pucData; //keep a copy of this member before updating it for markers calculation

	bool bDidLoop = false;
	AkUInt32 nADPCMBlocks = io_state.buffer.MaxFrames() / ADPCM_SAMPLES_PER_BLOCK; // number of ADPCM blocks to process to fill the output buffer
	while ( nADPCMBlocks )
	{
		if ( DoLoop() )
		{
			AkUInt32 nSrcBlocks = (AkUInt32) ( m_pucLoopEnd - m_pucData ) / m_usBlockAlign;
			if ( nADPCMBlocks < nSrcBlocks )
			{
				for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
					CAkADPCMCodec::Decode( m_pucData + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), nADPCMBlocks, m_usBlockAlign, uNumChannels );

				m_pucData += nADPCMBlocks * m_usBlockAlign;
				pOutBuffer += nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK * ulPCMBlockAlign;

				break;
			}
			else // loop around
			{
				for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
					CAkADPCMCodec::Decode( m_pucData + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), nSrcBlocks, m_usBlockAlign, uNumChannels );

				m_pucData = m_pucLoopStart;
				pOutBuffer += nSrcBlocks * ADPCM_SAMPLES_PER_BLOCK * ulPCMBlockAlign;

				nADPCMBlocks -= nSrcBlocks;

				if( m_uLoopCnt > 0 )
					m_uLoopCnt--;

				bDidLoop = true;
			}
		}
		else
		{
			AkUInt32 nSrcBlocks = (AkUInt32) ( ( m_pucDataStart + m_ulDataSize ) - m_pucData ) / m_usBlockAlign;
			if ( nADPCMBlocks >= nSrcBlocks )
			{
				nADPCMBlocks = nSrcBlocks;
				l_eResult = AK_NoMoreData;
			}

			for ( AkUInt32 iChan = 0; iChan < uNumChannels; ++iChan )
				CAkADPCMCodec::Decode( m_pucData + iChan * ADPCM_BLOCK_SIZE, pOutBuffer + iChan * sizeof(AkInt16), nADPCMBlocks, m_usBlockAlign, uNumChannels );

			m_pucData += nADPCMBlocks * m_usBlockAlign;
			pOutBuffer += nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK * ulPCMBlockAlign;

			break;
		}
	}

    AkUInt16 uValidFrames = (AkUInt16)( pOutBuffer - m_pOutBuffer ) / (AkUInt16)ulPCMBlockAlign;
    io_state.buffer.AttachInterleavedData( m_pOutBuffer, AK_NUM_VOICE_REFILL_FRAMES, uValidFrames, uChannelMask );

	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( pucData - m_pucDataStart ) * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign;

	CopyRelevantMarkers( io_state.buffer, l_ulCurrSampleOffset, bDidLoop );

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = l_pFormat->uSampleRate;
		io_state.buffer.posInfo.uStartPos = l_ulCurrSampleOffset;
		AKASSERT( (m_ulDataSize % m_usBlockAlign) == 0 );
		io_state.buffer.posInfo.uFileEnd = (m_ulDataSize / m_usBlockAlign) * ADPCM_SAMPLES_PER_BLOCK;
	}

	io_state.result = l_eResult;
}

AKRESULT CAkSrcBankADPCM::TimeSkip( AkUInt32 & io_uFrames )
{
	AKRESULT eResult = AK_DataReady;

	AkUInt32 uFramesOutput = 0;

	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( m_pucData - m_pucDataStart ) * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign;

	AkUInt32 nADPCMBlocks = io_uFrames / ADPCM_SAMPLES_PER_BLOCK; // number of ADPCM blocks to process to fill the output buffer
	while ( nADPCMBlocks )
	{
		if ( DoLoop() )
		{
			AkUInt32 nSrcBlocks = (AkUInt32) ( m_pucLoopEnd - m_pucData ) / m_usBlockAlign;
			if ( nADPCMBlocks < nSrcBlocks )
			{
				NotifyRelevantMarkers( l_ulCurrSampleOffset, nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK + l_ulCurrSampleOffset );
				m_pucData += nADPCMBlocks * m_usBlockAlign;
				uFramesOutput += nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK;

				break;
			}
			else // loop around
			{
				m_pucData = m_pucLoopStart;
				NotifyRelevantMarkers( l_ulCurrSampleOffset, nSrcBlocks * ADPCM_SAMPLES_PER_BLOCK + l_ulCurrSampleOffset );
				l_ulCurrSampleOffset = (AkUInt32) ( m_pucData - m_pucDataStart ) * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign;
				uFramesOutput += nSrcBlocks * ADPCM_SAMPLES_PER_BLOCK;

				nADPCMBlocks -= nSrcBlocks;

				if( m_uLoopCnt > 0 )
					m_uLoopCnt--;
			}
		}
		else
		{
			AkUInt32 nSrcBlocks = (AkUInt32) ( ( m_pucDataStart + m_ulDataSize ) - m_pucData ) / m_usBlockAlign;
			NotifyRelevantMarkers( l_ulCurrSampleOffset, nSrcBlocks * ADPCM_SAMPLES_PER_BLOCK - l_ulCurrSampleOffset );
			if ( nADPCMBlocks >= nSrcBlocks )
			{
				nADPCMBlocks = nSrcBlocks;
				eResult = AK_NoMoreData;
			}

			m_pucData += nADPCMBlocks * m_usBlockAlign;
			uFramesOutput += nADPCMBlocks * ADPCM_SAMPLES_PER_BLOCK;

			break;
		}
	}

    io_uFrames = uFramesOutput;

	UpdatePositionInfo( l_ulCurrSampleOffset, m_ulDataSize * ADPCM_SAMPLES_PER_BLOCK / m_usBlockAlign );

	return eResult;
}

void CAkSrcBankADPCM::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		m_pucData = m_pucDataStart;
		m_uLoopCnt = m_pCtx->GetLooping();
	}
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkSrcBankADPCM::GetDuration( void ) const
{
	AkAudioFormat * l_pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( l_pFormat != NULL );
    AKASSERT( m_pucLoopEnd >= m_pucLoopStart );

    AkUInt16 uNumLoops = m_pCtx->GetLooping( );
    if ( uNumLoops == 0 )
        return 0;

    AkReal32 l_fTotalSize = (AkReal32)m_ulDataSize + (AkReal32)(uNumLoops-1)*(AkUInt32)(m_pucLoopEnd-m_pucLoopStart);
	AkReal32 l_fSamples = l_fTotalSize/(AkReal32)m_usBlockAlign * ADPCM_SAMPLES_PER_BLOCK;
	AkReal32 l_fRate    = (AkReal32)l_pFormat->uSampleRate;

	return (AkTimeMs)((l_fSamples*1000.f)/l_fRate);		// mSec.
}

//-----------------------------------------------------------------------------
// Name: StopLooping()
// Desc: Play to end of data.
//
//
//-----------------------------------------------------------------------------
AKRESULT CAkSrcBankADPCM::StopLooping()
{
	m_uLoopCnt = 1;
	return AK_Success;
}
