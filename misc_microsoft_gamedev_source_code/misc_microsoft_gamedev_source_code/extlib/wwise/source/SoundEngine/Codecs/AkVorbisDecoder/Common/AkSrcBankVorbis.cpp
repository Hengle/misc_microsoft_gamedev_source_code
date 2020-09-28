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
#include "AkSrcBankVorbis.h"
#include <AK/Plugin/AkVorbisFactory.h>
#include "AkLEngine.h"
#include "AkFileParser.h"
#include "AkFileParserBase.h"
#include "AkMonitor.h"

// TODO: Tune this number...
#define INPUTDATASIZE (2*1024)
#define OUTPUT_CHANNEL_SIZE	(LE_MAX_FRAMES_PER_BUFFER * sizeof(AkInt16))

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
IAkSoftwareCodec* CreateVorbisBankPlugin( void* in_pCtx )
{
#ifdef WIN32	
	return AkNew( g_LEngineDefaultPoolId, CAkSrcBankVorbis( (CAkPBI*)in_pCtx, CAkLEngine::SSE2IsAvailable() ) );
#else
	return AkNew( g_LEngineDefaultPoolId, CAkSrcBankVorbis( (CAkPBI*)in_pCtx ) );
#endif
}

// Constructor
CAkSrcBankVorbis::CAkSrcBankVorbis( CAkPBI * in_pCtx
#ifdef WIN32
								   , bool in_bSSE2 
#endif
								   ) : CAkSrcBaseEx( in_pCtx )
{
	// Initially was set to INVALID_POOL_ID, now we know
	g_VorbisPoolId = g_LEngineDefaultPoolId;

	m_pucData			= NULL;		// current data pointer
	m_uDataSize			= 0;		// whole data size
	m_pucDataStart		= NULL;		// start of audio data
	m_pucBackBoundary	= NULL;		// 
#ifdef WIN32	
	m_VorbisState.TremorInfo.VorbisDSPState.bSSE2 = in_bSSE2;
#endif	

	// do this here as well as it is legal to be StopStream'ed
	// without having been StartStream'ed
	InitVorbisState();
}

// Destructor
CAkSrcBankVorbis::~CAkSrcBankVorbis()
{
	ReleaseBuffer();
}

// GetBuffer
void CAkSrcBankVorbis::GetBuffer( AkVPLState & io_state )
{
	// The stream should not start before all headers are decoded
	assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );

	// Allocate output buffer for decoded data
	if ( m_VorbisState.TremorInfo.pucData == NULL )
	{
		m_VorbisState.TremorInfo.pucData = (AkUInt8 *) CAkLEngine::GetCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE);
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		if ( m_VorbisState.TremorInfo.pucData == NULL )
		{
			io_state.result = AK_InsufficientMemory;
			return;
		}
	}

	// Note: No need to adjust request to for file end or looping as it will finish the decoding a packet anyhow
	m_VorbisState.TremorInfo.uRequestedFrames = io_state.buffer.MaxFrames();

	// Trim input data to loop end if necessary
	m_VorbisState.TremorInfo.uInputDataSize = INPUTDATASIZE;
	if ((m_pucData + m_VorbisState.TremorInfo.uInputDataSize) >= m_pucBackBoundary)
	{
		m_VorbisState.TremorInfo.uInputDataSize = m_pucBackBoundary - m_pucData;
		if ( !DoLoop() )
		{
			m_VorbisState.TremorInfo.bNoMoreInputPackets = true;
		}
	}
	m_VorbisState.TremorInfo.uInputDataOffset = 0;

	// Vorbis is ready, simply decode
	m_VorbisState.uBufferStartPCMFrame = m_VorbisState.uCurrentPCMFrame;
	DecodeVorbis( &m_VorbisState.TremorInfo, m_pucData, (AkInt16*)m_VorbisState.TremorInfo.pucData );
	// Advance m_pucData based on how many packets were consumed
	m_pucData += m_VorbisState.TremorInfo.ReturnInfo.uInputBytesConsumed;
	m_VorbisState.uCurrentPCMFrame += m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced;

	// Set transfer object values.
	/// NOTE (multichannel) If Vorbis were to implement multichannel, the real channel mask would need to be
	/// parsed from header instead of using the TremorInfo.
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
	if ( DoLoop() && m_pucData >= m_pucBackBoundary )
	{
		m_pucData = m_pucDataStart + m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartPacketOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		m_VorbisState.uPCMStartLimitMarkers = m_uPCMLoopStart;
		if ( m_uLoopCnt != LOOPING_INFINITE )
		{
			--m_uLoopCnt;
			if ( !DoLoop() )
			{
				m_pucBackBoundary = m_pucDataStart + m_uDataSize;
				// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
				// so make sure that uCurrentPCMFrame will never be bigger than this one
				m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
			}
		}

		// Re-Initialize global decoder state
		// vorbis_dsp_restart( &m_VorbisState.TremorInfo.VorbisDSPState );
		assert( m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset <= m_uPCMLoopStart );
		m_VorbisState.uCurrentPCMFrame = m_uPCMLoopStart - m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset;
	}

	io_state.result = m_VorbisState.TremorInfo.ReturnInfo.eDecoderStatus;
}

// ReleaseBuffer
void CAkSrcBankVorbis::ReleaseBuffer()
{
	if ( m_VorbisState.TremorInfo.pucData )
    {
		CAkLEngine::ReleaseCachedAudioBuffer( AK::GetNumChannels(m_VorbisState.TremorInfo.uChannelMask) * OUTPUT_CHANNEL_SIZE, m_VorbisState.TremorInfo.pucData );
		m_VorbisState.TremorInfo.ReturnInfo.uFramesProduced = 0;
		m_VorbisState.TremorInfo.pucData = NULL;
    }
}

// GetDuration
AkTimeMs CAkSrcBankVorbis::GetDuration( ) const
{
    return (AkTimeMs) ((m_VorbisState.VorbisInfo.dwTotalPCMFrames * 1000.f) / m_VorbisState.uSampleRate);	// mSec.
}

// StartStream
AKRESULT CAkSrcBankVorbis::StartStream( )
{
    assert( m_markers.m_hdrMarkers.uNumMarkers == 0 && m_markers.m_pMarkers == NULL ); 

	InitVorbisState( );

    AkUInt8 * pvBuffer;
    AkUInt32 ulBufferSize;
	m_pCtx->GetDataPtr( pvBuffer, ulBufferSize );
	if ( pvBuffer == NULL )
	{
		return AK_Fail;
	}

	AkUInt32 uDataOffset;
	WaveFormatEx fmt;
	AKRESULT eResult = CAkFileParser::Parse(pvBuffer,					// Data buffer
											ulBufferSize,				// Buffer size
											&fmt,						// Returned audio format.
											sizeof( WaveFormatEx ),
											&m_markers,         // Markers.
											&m_uPCMLoopStart,			// Beginning of loop offset.
											&m_uPCMLoopEnd,				// End of loop offset.
											&m_uDataSize,				// Data size.
											&uDataOffset,				// Offset to data.
											&m_VorbisState.VorbisInfo,	// Vorbis specific information
											sizeof (AkVorbisInfo) );	// Vorbis specific information size

	if ( eResult != AK_Success )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return eResult;
	}

	m_pucDataStart = pvBuffer + uDataOffset;
	m_VorbisState.uSampleRate = fmt.nSamplesPerSec;
	// TMP multichannel
	m_VorbisState.TremorInfo.uChannelMask = ( fmt.nChannels == 2 ) ? AK_SPEAKER_SETUP_STEREO : AK_SPEAKER_SETUP_MONO;

	// Fix loop start and loop end for no SMPL chunk
	if((m_uPCMLoopStart == 0) && (m_uPCMLoopEnd == 0))
	{	
		m_uPCMLoopEnd = m_VorbisState.VorbisInfo.dwTotalPCMFrames-1;
	}

	// Verify data buffer consistency.
	if ( ulBufferSize != (uDataOffset + m_uDataSize) )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_InvalidAudioFileHeader );
		return AK_Fail;
	}

	m_pucData = m_pucDataStart;
	LoopInit();

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

	if(!m_VorbisState.Allocator.Init(m_VorbisState.VorbisInfo.dwDecodeAllocSize))
	{
		return AK_Fail;
	}

	eResult = DecodeVorbisHeader( );

	if( eResult == AK_Success )
	{
		AkUInt64 l_numNativeSampleOffset = m_pCtx->GetSourceOffset();
		if( l_numNativeSampleOffset && m_VorbisState.pSeekTable )
		{
			AkUInt64 l_numRealSampleOffset = l_numNativeSampleOffset * m_pCtx->GetMediaFormat()->uSampleRate / AK_CORE_SAMPLERATE;

			AkUInt32 uRealOffset = l_numRealSampleOffset;
			eResult = VirtualSeek( uRealOffset );
			if( eResult != AK_Success )
			{
				return eResult;
			}

			assert( l_numRealSampleOffset - uRealOffset >= 0 );
			m_pCtx->SetSourceOffset( l_numRealSampleOffset - uRealOffset );
		}
	}

	return eResult;
}

// StopStream
void CAkSrcBankVorbis::StopStream( )
{
	TermVorbisState();

	ReleaseBuffer();
	if ( m_VorbisState.pSeekTable )
	{
		AkFree( g_VorbisPoolId, m_VorbisState.pSeekTable );
		m_VorbisState.pSeekTable = NULL;
	}



	m_VorbisState.Allocator.Term();

	CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcBankVorbis::StopLooping()
{
	m_uLoopCnt = 1;
	// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
	// so make sure that uCurrentPCMFrame will never be bigger than this one
	m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;

	return AK_Success;
}

// VirtualOn
void CAkSrcBankVorbis::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	// Nothing to do
}

// VirtualOff
AKRESULT CAkSrcBankVorbis::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromBeginning || eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );
		// Re-Initialize global decoder state
		vorbis_dsp_restart( &m_VorbisState.TremorInfo.VorbisDSPState );
		m_VorbisState.TremorInfo.ReturnInfo.bPacketLeftOver = false;
	}

	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );
		return VirtualSeek( m_VorbisState.uCurrentPCMFrame );
	}
	else if ( eBehavior == AkVirtualQueueBehavior_FromBeginning ) 
	{
		assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );
		// Setup completed go to data directly
		m_pucData = m_pucDataStart + m_VorbisState.VorbisInfo.dwVorbisDataOffset;
		LoopInit();
	}

	// Nothing to do for resume mode
	return AK_Success;
}

// Time skip
AKRESULT CAkSrcBankVorbis::TimeSkip( AkUInt32 & io_uFrames )
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
			m_VorbisState.uCurrentPCMFrame = m_uPCMLoopStart - m_VorbisState.VorbisInfo.LoopInfo.dwLoopStartFrameOffset;
			if( m_uLoopCnt != LOOPING_INFINITE )
			{
				--m_uLoopCnt;
				if ( !DoLoop() )
				{
					m_pucBackBoundary = m_pucDataStart + m_uDataSize;
					m_VorbisState.uPCMStartLimitMarkers = m_uPCMLoopStart;
					// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
					// so make sure that uCurrentPCMFrame will never be bigger than this one
					m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
				}
			}
        }
        else
        {
			TimeSkipMarkers( m_VorbisState.uBufferStartPCMFrame, io_uFrames, m_VorbisState.VorbisInfo.dwTotalPCMFrames );
        	
			// Hit the end.
			return AK_NoMoreData;
        }
	}

	TimeSkipMarkers( m_VorbisState.uBufferStartPCMFrame, io_uFrames, m_VorbisState.VorbisInfo.dwTotalPCMFrames );

	return AK_DataReady;
}

// VirtualSeek
// Determine where to seek to using seek table
AKRESULT CAkSrcBankVorbis::VirtualSeek( AkUInt32 & io_uSeekPosition )
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

		m_pucData = m_pucDataStart + m_VorbisState.pSeekTable[index].dwPacketFileOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		io_uSeekPosition = m_VorbisState.pSeekTable[index].dwPacketFirstPCMFrame;

		vorbis_dsp_restart( &m_VorbisState.TremorInfo.VorbisDSPState );
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
AKRESULT CAkSrcBankVorbis::DecodeVorbisHeader()
{
	AkInt32 iResult;
	if ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets == UNINITIALIZED )
	{
		AKRESULT eResult = InitVorbisInfo();
		if(eResult != AK_Success)
		{
			return eResult;
		}
	}

	// Try to get the setup, comment and codebook headers and set up the Vorbis decoder
	// Any error while decoding header is fatal.
	// Read seek table
	if ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < SEEKTABLEINTIALIZED )
	{
		if ( m_VorbisState.bNeedSeekTable )
		{
			// Read all seek table items
			assert( m_VorbisState.pSeekTable != NULL && m_VorbisState.VorbisInfo.dwSeekTableSize > 0 );	
			AkMemCpy( m_VorbisState.pSeekTable, m_pucData, m_VorbisState.VorbisInfo.dwSeekTableSize ); 	
		}
		// Skip over unnecessary seek table otherwise
		m_pucData += m_VorbisState.VorbisInfo.dwSeekTableSize;
		m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets = SEEKTABLEINTIALIZED;
	}

	// Read Vorbis header packets
	while ( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets < CODEBOOKHEADERDECODED ) 
	{
		// Get the next packet
		ogg_packet Packet;
		UnpackPacket(m_pucData,m_pucData+sizeof(OggPacketHeader),m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets,false,Packet);
		m_pucData += sizeof(OggPacketHeader) + Packet.buffer.size;
		++m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets;
		// Synthesize Vorbis header
		iResult = vorbis_dsp_headerin(
			&(m_VorbisState.TremorInfo.VorbisDSPState.vi),
			&Packet,
			m_VorbisState.Allocator );
		if ( iResult )
		{
			// DO NOT ASSERT! Can fail because of failed _ogg_malloc() in merge_sort. assert( !"Failure synthesizing setup header." );
			return AK_Fail;
		}
	}

	// Only get here once all three headers are parsed, complete Vorbis setup
	assert( m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets >= CODEBOOKHEADERDECODED );

	// Initialize global decoder state
	iResult = vorbis_dsp_init( &(m_VorbisState.TremorInfo.VorbisDSPState),m_VorbisState.Allocator );
	if ( iResult )
	{
		// DO NOT ASSERT! Can fail because of failed _ogg_malloc(). assert( !"Failure initializing Vorbis decoder." );
		return AK_Fail;
	}

	return AK_Success;
}

void CAkSrcBankVorbis::InitVorbisState( )
{
#ifdef WIN32
	bool bSSE2 = m_VorbisState.TremorInfo.VorbisDSPState.bSSE2;
#endif
	memset(&m_VorbisState, 0, sizeof(AkVorbisSourceState));
	m_VorbisState.TremorInfo.ReturnInfo.iDecodedPackets = UNINITIALIZED;
#ifdef WIN32
	m_VorbisState.TremorInfo.VorbisDSPState.bSSE2 = bSSE2;
#endif
}

AKRESULT CAkSrcBankVorbis::TermVorbisState( )
{
	vorbis_dsp_clear( &m_VorbisState.TremorInfo.VorbisDSPState, m_VorbisState.Allocator);
	return AK_Success;
}

AKRESULT CAkSrcBankVorbis::InitVorbisInfo()
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

void CAkSrcBankVorbis::LoopInit()
{
	m_VorbisState.uCurrentPCMFrame = 0;

	// Init state.
	m_uLoopCnt = m_pCtx->GetLooping();	// Current loop count.
	m_VorbisState.uPCMStartLimitMarkers = 0; // Always consider frames for markers until it has looped once
	if ( !DoLoop() )
	{
		m_pucBackBoundary = m_pucDataStart + m_uDataSize;
		// as vorbis is not sample accurate we may overshoot dwTotalPCMFrames
		// so make sure that uCurrentPCMFrame will never be bigger than this one
		m_VorbisState.uPCMEndLimitMarkers = 0xFFFFFFFF;
	}
	else
	{
		// Seek table is inserted in front of encoded data so we need to offset by SeekTable size
		m_pucBackBoundary = m_pucDataStart + m_VorbisState.VorbisInfo.LoopInfo.dwLoopEndPacketOffset + m_VorbisState.VorbisInfo.dwSeekTableSize;
		m_VorbisState.uPCMEndLimitMarkers = m_uPCMLoopEnd; // Consider frames up to loop end for markers
	}
}
