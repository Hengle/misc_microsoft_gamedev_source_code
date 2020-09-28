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
#include "AkSrcBankPCM.h"
#include "AkFileParserBase.h"
#include "AkFileParser.h"
#include "AkMonitor.h"
#include "IAkMotionMixBus.h"

//-----------------------------------------------------------------------------
// Name: CAkSrcBankPCM
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankPCM::CAkSrcBankPCM( CAkPBI * in_pCtx )
: CAkSrcBaseEx( in_pCtx )
, m_pucData( NULL )
, m_pucLoopStart( NULL )
, m_pucLoopEnd( NULL )
, m_ulDataSize( 0 )
, m_pucDataStart( NULL )
{
}

//-----------------------------------------------------------------------------
// Name: ~CAkSrcBankPCM
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcBankPCM::~CAkSrcBankPCM()
{
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
AKRESULT CAkSrcBankPCM::StartStream()
{
    AKASSERT( m_pucDataStart == NULL );

    AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
    if ( pvBuffer == NULL )
		return AK_Fail;

    AkUInt32 uDataOffset;
    AKRESULT eResult;

	WaveFormatEx fmt;

	eResult = CAkFileParser::Parse( pvBuffer,      // Data buffer
									ulBufferSize,  // Buffer size
									&fmt,  // Audio format
									sizeof( WaveFormatEx ),
									&m_markers,         // Markers.
									&m_uPCMLoopStart,       // Beginning of loop offset.
									&m_uPCMLoopEnd,			// End of loop offset.
									&m_ulDataSize,		// Data size.
									&uDataOffset,	// Offset to data.
									NULL,				// Fmt specific information
									NULL );				// Fmt specific information size

    if ( eResult != AK_Success )
    {
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_InvalidFile;
    }

	AKASSERT( uDataOffset % 4 == 0 );

	m_pucDataStart = pvBuffer + uDataOffset;
	m_usBlockAlign = fmt.nBlockAlign;

	// Parsed loop start and end are relative to start of data chunk, in sample frames. 
	// Set to absolute start and end in bytes.
	m_pucLoopStart = m_pucDataStart + m_usBlockAlign * m_uPCMLoopStart;
	// Set Loop point to end of file if not looping sound or if there are no loop points.
	// Note. If there are no loop points, LoopEnd is set to 0. 
	if ( m_uPCMLoopEnd == 0 || m_pCtx->GetLooping() == 1 )
	{
		// No loop point. (Loop points inclusive)
		m_pucLoopEnd = m_pucDataStart + m_ulDataSize;
	}
	else
	{
		// Loop points: translate.
		m_pucLoopEnd = m_pucDataStart + m_usBlockAlign * (m_uPCMLoopEnd+1);
	}
	
	// Verify data buffer consistency.
	if ( m_pucLoopEnd < m_pucLoopStart ||
		m_pucLoopStart > (m_pucDataStart + m_ulDataSize) || 
		m_pucLoopEnd > (m_pucDataStart + m_ulDataSize) || 
		( ulBufferSize != (uDataOffset + m_ulDataSize) ) )
	{
        MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_Fail;
	}

	// Init state.

	AkUInt32 lSampleRate = m_pCtx->IsForFeedbackPipeline() ? AK_FEEDBACK_SAMPLE_RATE : AK_CORE_SAMPLERATE;
	AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
	m_pCtx->SetSourceOffset( 0 );
	AkUInt64 l_numRealSampleOffset = l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / lSampleRate;
	
	m_pucData = m_pucDataStart + (AkUInt32)( l_numRealSampleOffset * m_usBlockAlign );// Beginning of data + Initial Offset
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
void CAkSrcBankPCM::StopStream()
{
    // Clean up.
	m_pucDataStart = NULL;
	CAkSrcBaseEx::StopStream();
}

void CAkSrcBankPCM::GetBuffer( AkVPLState & io_state )
{	
	AkAudioFormat * pFormat = m_pCtx->GetMediaFormat();
	AkChannelMask uChannelMask = pFormat->GetChannelMask();
	
	AKRESULT eResult = AK_DataReady;

	AkUInt16 uMaxFrames = io_state.buffer.MaxFrames();
	AkUInt32 uReqSize = uMaxFrames * m_usBlockAlign;
	AkUInt8 * pucLimit = DoLoop() ? m_pucLoopEnd : m_pucDataStart + m_ulDataSize;
	if ( uReqSize > (AkUInt32) ( pucLimit - m_pucData ) )
	{
		uReqSize = (AkUInt32) ( pucLimit - m_pucData );
		uMaxFrames = (AkUInt16) ( uReqSize / m_usBlockAlign );
	}

    io_state.buffer.AttachInterleavedData( m_pucData, uMaxFrames, uMaxFrames, uChannelMask );

	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( m_pucData - m_pucDataStart ) / m_usBlockAlign;
	CopyRelevantMarkers( io_state.buffer, l_ulCurrSampleOffset );

	if( m_markers.NeedPositionInformation() )
	{
		io_state.buffer.posInfo.uSampleRate = pFormat->uSampleRate;
		io_state.buffer.posInfo.uStartPos = l_ulCurrSampleOffset;
		io_state.buffer.posInfo.uFileEnd = m_ulDataSize / m_usBlockAlign;
	}

	m_pucData += uReqSize;

	if ( m_pucData == pucLimit )
	{
	    if ( DoLoop() )
		{
			m_pucData = m_pucLoopStart;
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
		}
		else
		{
			eResult = AK_NoMoreData;
		}
	}

	io_state.result = eResult;
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Release a specified buffer.
//-----------------------------------------------------------------------------
void CAkSrcBankPCM::ReleaseBuffer()
{
}

AKRESULT CAkSrcBankPCM::TimeSkip( AkUInt32 & io_uFrames )
{
	AKRESULT eResult = AK_DataReady;

	AkUInt32 uReqSize = io_uFrames * m_usBlockAlign;
	AkUInt8 * pucLimit = DoLoop() ? m_pucLoopEnd : m_pucDataStart + m_ulDataSize;
	if ( uReqSize > (AkUInt32) ( pucLimit - m_pucData ) )
	{
		uReqSize = (AkUInt32) ( pucLimit - m_pucData );
		io_uFrames = uReqSize / m_usBlockAlign;
	}

	AkUInt32 l_ulCurrSampleOffset = (AkUInt32) ( m_pucData - m_pucDataStart ) / m_usBlockAlign;
	TimeSkipMarkers( l_ulCurrSampleOffset, io_uFrames, m_ulDataSize / m_usBlockAlign );

	m_pucData += uReqSize;

	if ( m_pucData == pucLimit )
	{
	    if ( DoLoop() )
		{
			m_pucData = m_pucLoopStart;
			if( m_uLoopCnt > 0 )
				m_uLoopCnt--;
		}
		else
		{
			eResult = AK_NoMoreData;
		}
	}

	return eResult;
}

void CAkSrcBankPCM::VirtualOn( AkVirtualQueueBehavior eBehavior )
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
AkTimeMs CAkSrcBankPCM::GetDuration() const
{
	//Maths must be done in float, 32bits will overflow too easily.
	AkAudioFormat * l_pFormat = m_pCtx->GetMediaFormat();
	AKASSERT( l_pFormat != NULL );
    AKASSERT( m_pucLoopEnd >= m_pucLoopStart );

    AkUInt16 uNumLoops = m_pCtx->GetLooping( );
    if ( uNumLoops == 0 )
        return 0;

    AkReal32 l_fTotalSize = (AkReal32)m_ulDataSize + (AkReal32)(uNumLoops-1)*(m_pucLoopEnd-m_pucLoopStart);
	AkReal32 l_fSamples = l_fTotalSize / (AkReal32)l_pFormat->GetBlockAlign();
	AkReal32 l_fRate    = (AkReal32)(l_pFormat->uSampleRate);

    return (AkTimeMs)((l_fSamples*1000.f)/l_fRate);		// mSec.
}

//-----------------------------------------------------------------------------
// Name: StopLooping()
// Desc: Play to end of data.
//
//-----------------------------------------------------------------------------
AKRESULT CAkSrcBankPCM::StopLooping()
{
	m_uLoopCnt = 1;
	return AK_Success;
}
