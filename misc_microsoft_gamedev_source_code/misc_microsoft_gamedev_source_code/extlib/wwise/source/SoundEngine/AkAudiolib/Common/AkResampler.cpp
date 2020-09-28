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
// AkResampler.cpp
// 
// Combines software resampling and pitch shifting opreation in one algorithm 
// using linear interpolation.
// Assumes same thread will call both SetPitch and Execute (not locking).
// There is some interpolation done on the pitch control parameter do avoid stepping behavior in transition
// or fast parameter changes.
// 
// We can think of latency / stepping problem by looking at rates at which different thinks occur:
// Control rate: rate at which SetPitch are called -> 1x per buffer (linked with buffer time)
// Interpolation rate: rate at which transitional pitch values are changed 
// -> NUMBLOCKTOREACHTARGET per buffer, necessary to avoid stepping while introduces up to 1 buffer latency
// Audio rate: rate at which samples are calculated == sample rate
// Simplifying assumption -> if its bypassed, its bypassed for the whole buffer
// It is possible to run the pitch algorithm with pitch 0.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkResampler.h"
#include "AkResamplerCommon.h"
#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkCommon.h"
#ifdef PERFORMANCE_BENCHMARK
#include "AkMonitor.h"
#endif

#define AK_LE_MIN_PITCHSHIFTCENTS (-2400.f)
#define AK_LE_MAX_PITCHSHIFTCENTS (2400.f)

#ifdef AK_PS3

#include "AkLEngine.h"

extern char _binary_Bypass_I16_spu_bin_start[];
extern char _binary_Bypass_I16_spu_bin_size[];
extern char _binary_Bypass_U8_spu_bin_start[];
extern char _binary_Bypass_U8_spu_bin_size[];
extern char _binary_Bypass_Native_spu_bin_start[];
extern char _binary_Bypass_Native_spu_bin_size[];
extern char _binary_Fixed_I16_spu_bin_start[];
extern char _binary_Fixed_I16_spu_bin_size[];
extern char _binary_Fixed_U8_spu_bin_start[];
extern char _binary_Fixed_U8_spu_bin_size[];
extern char _binary_Fixed_Native_spu_bin_start[];
extern char _binary_Fixed_Native_spu_bin_size[];
extern char _binary_Interpolating_I16_spu_bin_start[];
extern char _binary_Interpolating_I16_spu_bin_size[];
extern char _binary_Interpolating_U8_spu_bin_start[];
extern char _binary_Interpolating_U8_spu_bin_size[];
extern char _binary_Interpolating_Native_spu_bin_start[];
extern char _binary_Interpolating_Native_spu_bin_size[];

static AK::MultiCoreServices::BinData JobBinInfo[NumPitchOperatingMode][NumInputDataType/4] = 
{
	{
		{ _binary_Bypass_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Bypass_I16_spu_bin_size ) },
		{ _binary_Bypass_U8_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Bypass_U8_spu_bin_size ) },
		{ _binary_Bypass_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Bypass_Native_spu_bin_size ) },
	},
	{
		{ _binary_Fixed_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Fixed_I16_spu_bin_size ) },
		{ _binary_Fixed_U8_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Fixed_U8_spu_bin_size ) },
		{ _binary_Fixed_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Fixed_Native_spu_bin_size ) },
	},
	{
		{ _binary_Interpolating_I16_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Interpolating_I16_spu_bin_size ) },
		{ _binary_Interpolating_U8_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Interpolating_U8_spu_bin_size ) },
		{ _binary_Interpolating_Native_spu_bin_start, CELL_SPURS_GET_SIZE_BINARY( _binary_Interpolating_Native_spu_bin_size ) },
	}
};

#else

typedef AKRESULT (*PitchDSPFuncPtr) (	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState );

static PitchDSPFuncPtr PitchDSPFuncTable[NumPitchOperatingMode][NumInputDataType] = 
{
	{
		//Note: 1 channel cases handled by N routine, stereo is faster with deinterleaving stage built-in
		Bypass_I16_NChanVec,
		Bypass_I16_2Chan,	
		Bypass_I16_NChanVec,
		Bypass_I16_NChanVec,
		Bypass_U8_NChanVec,
		Bypass_U8_2Chan,
		Bypass_U8_NChanVec,
		Bypass_U8_NChanVec,
		// Note: 1Chan and 2 channel cases handled by N routine
		Bypass_Native_NChan,
		Bypass_Native_NChan,
		Bypass_Native_NChan,
		Bypass_Native_NChan,
	},
#ifdef WIN32
	{
		Fixed_I16_1Chan,
		Fixed_I16_2Chan,
		Fixed_I16_NChan,
		Fixed_I16_NChan,
		Fixed_U8_1Chan,
		Fixed_U8_2Chan,
		Fixed_U8_NChan,
		Fixed_U8_NChan,
		Fixed_Native_1Chan,
		Fixed_Native_2Chan,
		Fixed_Native_NChan,
		Fixed_Native_NChan,
	},
	{
		Interpolating_I16_1Chan,
		Interpolating_I16_2Chan,
		Interpolating_I16_NChan,
		Interpolating_I16_NChan,
		Interpolating_U8_1Chan,
		Interpolating_U8_2Chan,
		Interpolating_U8_NChan,
		Interpolating_U8_NChan,
		Interpolating_Native_1Chan,
		Interpolating_Native_2Chan,
		Interpolating_Native_NChan,
		Interpolating_Native_NChan,
	}
#else
	{
		Fixed_I16_1Chan,
		Fixed_I16_2Chan,
		Fixed_I16_1To4ChanVec,
		Fixed_I16_5To8ChanVec,
		Fixed_U8_1Chan,
		Fixed_U8_2Chan,
		Fixed_U8_1To4ChanVec,
		Fixed_U8_5To8ChanVec,
		Fixed_Native_1Chan, 
		Fixed_Native_2Chan, 
		Fixed_Native_NChan_Vec,
		Fixed_Native_NChan_Vec,
	},
	{
		Interpolating_I16_1Chan,
		Interpolating_I16_2Chan,
		Interpolating_I16_1To4ChanVec,
		Interpolating_I16_5To8ChanVec,
		Interpolating_U8_1Chan,
		Interpolating_U8_2Chan,
		Interpolating_U8_1To4ChanVec,
		Interpolating_U8_5To8ChanVec,
		Interpolating_Native_1Chan,
		Interpolating_Native_2Chan,
		Interpolating_Native_NChan_Vec,
		Interpolating_Native_NChan_Vec,
	}
#endif

};

#endif


// Constructor
CAkResampler::CAkResampler( )
{
}

// Destructor
CAkResampler::~CAkResampler( )
{

}

// Frame skip includes resampling ratio frameskip = 1 / (pitchratio * (target sr)/(src sr))
// 1 / pitch ratio = frameskip / ( (src sr) / (target sr) )
AkReal32 CAkResampler::GetLastRate() 
{ 
	return ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) / m_fSampleRateConvertRatio;
}

// Pass on internal pitch state (must be called after Init)
void CAkResampler::SetPitchState( AkSharedPitchState & rPitchState )
{
	// Convert past samples to actual format of this source
	if  ( ISU8TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			m_InternalPitchState.uLastValue[i] = FLOAT_TO_UINT8( rPitchState.fLastBufVal[i] );
		}
	}
	else if  ( ISI16TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			m_InternalPitchState.iLastValue[i] = FLOAT_TO_INT16( rPitchState.fLastBufVal[i] );
		}
	}
	else if  ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			m_InternalPitchState.fLastValue[i] = rPitchState.fLastBufVal[i];
		}
	}
	else
	{
		AKASSERT( "Unsupported format." );
	}

	m_InternalPitchState.uFloatIndex = rPitchState.uFloatIndex;							// Fixed point index value
	m_InternalPitchState.uCurrentFrameSkip = rPitchState.uCurrentFrameSkip;				// Current sample frame skip
	m_InternalPitchState.uTargetFrameSkip = rPitchState.uTargetFrameSkip;				// Target frame skip
	m_InternalPitchState.uInterpolationRampCount = rPitchState.uInterpolationRampCount;	// Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)
	m_fTargetPitchVal = rPitchState.fTargetPitchVal;									// Target pitch value

	// force new pitch calculation and no interpolation when sample rate differs
	if ( m_fSampleRateConvertRatio != rPitchState.fSampleRateConvertRatio )
		m_bFirstSetPitch = true;
	else
		m_bFirstSetPitch = rPitchState.bFirstSetPitch;	// Flags first set pitch received
}

// Retrieve internal pitch state
void CAkResampler::GetPitchState( AkSharedPitchState & rPitchState )
{
	// Return zeros for channels we don<t have in case state is transfered to an instance with more channels.
	AKPLATFORM::AkMemSet( rPitchState.fLastBufVal, 0, AK_VOICE_MAX_NUM_CHANNELS*sizeof(AkReal32) );

	// Convert past samples to to native format
	if  ( ISU8TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			rPitchState.fLastBufVal[i] = UINT8_TO_FLOAT( m_InternalPitchState.uLastValue[i] );
		}
	}
	else if  ( ISI16TYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			rPitchState.fLastBufVal[i] = INT16_TO_FLOAT( m_InternalPitchState.iLastValue[i] );
		}
	}
	else if  ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			rPitchState.fLastBufVal[i] = m_InternalPitchState.fLastValue[i];
		}
	}
	else
	{
		AKASSERT( "Unsupported format." );
	}

	rPitchState.uFloatIndex = m_InternalPitchState.uFloatIndex;							// Fixed point index value
	rPitchState.uCurrentFrameSkip = m_InternalPitchState.uCurrentFrameSkip;				// Current sample frame skip
	rPitchState.uTargetFrameSkip = m_InternalPitchState.uTargetFrameSkip;				// Target frame skip
	rPitchState.uInterpolationRampCount = m_InternalPitchState.uInterpolationRampCount;// Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)
	rPitchState.fTargetPitchVal = m_fTargetPitchVal;									// Target pitch value
	rPitchState.fSampleRateConvertRatio = m_fSampleRateConvertRatio;
	rPitchState.bFirstSetPitch = m_bFirstSetPitch;		// Flags first set pitch received	
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Setup converter to use appropriate conversion routine
//-----------------------------------------------------------------------------
AKRESULT CAkResampler::Init( AkAudioFormat * io_pFormat, AkUInt32 in_usSampleRate )
{ 
	m_InternalPitchState.uOutFrameOffset = 0;
	m_InternalPitchState.uInFrameOffset = 0;

	m_InternalPitchState.uFloatIndex = SINGLEFRAMEDISTANCE; // Initial index set to 1 -> 0 == previous buffer
	// Note: No need to set last values since float index initial value is 1 

	// Pitch interpolation variables
	m_InternalPitchState.uCurrentFrameSkip = 0;				// Current frame skip
	m_InternalPitchState.uTargetFrameSkip = 0;				// Target frame skip
	m_InternalPitchState.uInterpolationRampCount = 0;		// Sample count for pitch parameter interpolation 
	m_fTargetPitchVal = 0.0f;								// Target pitch value
	m_bFirstSetPitch = true;

	// Set resampling ratio
	m_fSampleRateConvertRatio = (AkReal32) io_pFormat->uSampleRate / in_usSampleRate;
	m_uNumChannels = (AkUInt8)AK::GetNumChannels( io_pFormat->GetChannelMask() );
	m_uInputBlockAlign = (AkUInt8)io_pFormat->GetBlockAlign();
	AKASSERT( m_uNumChannels <= 8 );

	switch ( io_pFormat->GetBitsPerSample() )
	{
		case 16:
			AKASSERT( io_pFormat->GetInterleaveID() == AK_INTERLEAVED );
			switch ( m_uNumChannels )
			{
			case 1:
				m_DSPFunctionIndex = 0;
				break;
			case 2:
				m_DSPFunctionIndex = 1;
				break;
			case 3:
			case 4:
				m_DSPFunctionIndex = 2;
				break;
			case 5:
			case 6:
				m_DSPFunctionIndex = 3;
				break;
			default:
				AKASSERT( "Unsupported channel configuration." );
				return AK_Fail;
			}
			break;
		case 8:
			AKASSERT( io_pFormat->GetInterleaveID() == AK_INTERLEAVED );
			switch ( m_uNumChannels )
			{
			case 1:
				m_DSPFunctionIndex = 4;
				break;
			case 2:
				m_DSPFunctionIndex = 5;
				break;
			case 3:
			case 4:
				m_DSPFunctionIndex = 6;
				break;
			case 5:
			case 6:
				m_DSPFunctionIndex = 7;
				break;
			default:
				AKASSERT( "Unsupported channel configuration." );
				return AK_Fail;
			}
			break;
		case 32:
			AKASSERT( io_pFormat->GetInterleaveID() == AK_NONINTERLEAVED );
			switch ( m_uNumChannels )
			{
			case 1:
				m_DSPFunctionIndex = 8;
				break;
			case 2:
				m_DSPFunctionIndex = 9;
				break;
			case 3:
			case 4:
				m_DSPFunctionIndex = 10;
				break;
			case 5:
			case 6:
				m_DSPFunctionIndex = 11;
				break;
			default:
				AKASSERT( "Unsupported channel configuration." );
				return AK_Fail;
			}
			break;
		default:
			AKASSERT( !"Invalid sample resolution." );
			return AK_Fail;
	}
	m_PitchOperationMode = PitchOperatingMode_Bypass; // Will be set every time by SetPitch()

#ifdef PERFORMANCE_BENCHMARK
	m_fTotalTime = 0.f;
	m_uNumberCalls = 0;
#endif 

	return AK_Success;
}

#ifdef AK_PS3 

void CAkResampler::ExecutePS3(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								struct AkVPLState & io_state )
{
	AK::MultiCoreServices::DspProcess * pDsp = CAkLEngine::GetDspProcess();

	// Setup DSP process
	pDsp->ResetDspProcess( false );
	pDsp->SetDspProcess( JobBinInfo[m_PitchOperationMode][m_DSPFunctionIndex/4] );

	m_InternalPitchState.uRequestedFrames = uRequestedSize;
	m_InternalPitchState.uInValidFrames = io_pInBuffer->uValidFrames;
	m_InternalPitchState.uOutValidFrames = 0;
	m_InternalPitchState.eState = AK_Fail;
	m_InternalPitchState.uChannelMask = io_pInBuffer->GetChannelMask();
	m_InternalPitchState.uOutMaxFrames = io_pOutBuffer->MaxFrames();
	
	AkUInt8 * pInputStartAddress;
	if ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		// Output buffer (first channel position, others are offset by MaxFrames)
		AKASSERT( io_pInBuffer->MaxFrames() == io_pOutBuffer->MaxFrames() );
		pInputStartAddress = (AkUInt8 *)( (AkReal32*)io_pInBuffer->GetDataStartDMA() + m_InternalPitchState.uInFrameOffset ); // used to calculate input offset	
	}
	else
	{
		pInputStartAddress = (AkUInt8 *)io_pInBuffer->GetInterleavedData() + m_InternalPitchState.uInFrameOffset * m_uInputBlockAlign;	
	}

	m_InternalPitchState.uInOffset = (AkUInt32)pInputStartAddress & 0xF;
	// Output buffer (first channel position, others are offset by MaxFrames)
	m_InternalPitchState.pOutBuffer = (AkReal32*)io_pOutBuffer->GetDataStartDMA( ) + m_InternalPitchState.uOutFrameOffset;

	// Parameter packaging
	pDsp->AddDspProcessDma( &m_InternalPitchState, sizeof(AkInternalPitchState) );

	if ( ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		// Deinterleaved input buffer, use 1 DMA per channel
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			// Note: This works because uInOffset is the same for all channel ( a consequence that the channels are separated by a 16 byte multiple )
			AkUInt8* pDmaStart = (AkUInt8*) ((AkUInt32)( io_pInBuffer->GetChannel( i ) + m_InternalPitchState.uInFrameOffset ) & ~0xf );
			AkUInt32 uDmaSize = ALIGN_SIZE_16( io_pInBuffer->uValidFrames * sizeof(AkReal32) + m_InternalPitchState.uInOffset );
			pDsp->AddDspProcessDma( pDmaStart, uDmaSize );
		}
	}
	else
	{
		// Interleaved input buffer, use one or 2 DMAs dependent on size
		AkUInt8 * pDmaStart = (AkUInt8 *) ( (AkUInt32) pInputStartAddress & ~0xf ); // dma is 16-aligned
		AkUInt32 uDmaSize = ALIGN_SIZE_16( io_pInBuffer->uValidFrames * m_uInputBlockAlign + m_InternalPitchState.uInOffset );
		pDsp->AddDspProcessDma( pDmaStart, uDmaSize );
	}

	// Allocate worst-case scenario output buffer (SPU)
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();
	// TODO: Local storage could be reduced to (uMaxFrames-m_InternalPitchState.uOutFrameOffset)*io_pInBuffer->NumChannels()*sizeof(AkReal32) + uOutOffset 
	AkUInt32 uOutputSize = io_pInBuffer->NumChannels() * uMaxFrames * sizeof( AkReal32 );
	AKASSERT( uOutputSize % 16 == 0 );
	pDsp->SetOutputBufferSize( uOutputSize );

	io_state.result = AK_ProcessNeeded;
}

#else

//-----------------------------------------------------------------------------
// Name: Execute
//-----------------------------------------------------------------------------
AKRESULT CAkResampler::Execute(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize )
{
	AKASSERT( io_pInBuffer != NULL );
	AKASSERT( io_pOutBuffer != NULL );
	AKASSERT( uRequestedSize <= io_pOutBuffer->MaxFrames() );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeBefore;
	AKPLATFORM::PerformanceCounter( &TimeBefore ); 
#endif

	// Call appropriate DSP function
	AKRESULT eResult;
	do
	{
		eResult = (PitchDSPFuncTable[m_PitchOperationMode][m_DSPFunctionIndex])( io_pInBuffer, io_pOutBuffer, uRequestedSize, &m_InternalPitchState );
		if ( m_PitchOperationMode == PitchOperatingMode_Interpolating && m_InternalPitchState.uInterpolationRampCount  >= PITCHRAMPLENGTH )
		{
			m_InternalPitchState.uCurrentFrameSkip = m_InternalPitchState.uTargetFrameSkip;
			m_PitchOperationMode = PitchOperatingMode_Fixed;
			// Note: It is ok to go to fixed mode (even if it should have gone to bypass mode) for the remainder of this buffer
			// It will go back to bypass mode after next SetPitch() is called
		}
		
	} 
	while( io_pInBuffer->uValidFrames > 0 && 
		   io_pOutBuffer->uValidFrames < uRequestedSize );

#ifdef PERFORMANCE_BENCHMARK
	AkInt64 TimeAfter;
	AKPLATFORM::PerformanceCounter( &TimeAfter );
	AkReal32 fElapsed = AKPLATFORM::Elapsed( TimeAfter, TimeBefore );
	m_fTotalTime += fElapsed;
	++m_uNumberCalls;
#endif

	return eResult;
}



AKRESULT CAkResampler::DeinterleaveAndSwapOutputIfRequired( AkAudioBuffer * io_pIOBuffer )
{
#if defined(WIN32) || defined(XBOX360)

	// Note: Only multi-channel bypass paths used de-interleaving in post stage (non-native only) for Windows
	if (
#ifdef WIN32		
			m_PitchOperationMode == PitchOperatingMode_Bypass &&
#endif			
			m_uNumChannels > 2 && 
			!ISNATIVETYPE( m_DSPFunctionIndex ) )
	{
		// Do this in a temporary allocated buffer and release the one provided (buffer swap) once its done).
		AkPipelineBuffer DeinterleavedBuffer = *((AkPipelineBuffer*)io_pIOBuffer);
		AKRESULT eStatus = DeinterleavedBuffer.GetCachedBuffer( io_pIOBuffer->MaxFrames(), io_pIOBuffer->GetChannelMask() );
		if ( eStatus != AK_Success )
		{
			return eStatus;
		}

		DeinterleavedBuffer.uValidFrames = io_pIOBuffer->uValidFrames;
		Deinterleave_Native_NChan( io_pIOBuffer, &DeinterleavedBuffer, &m_InternalPitchState );

		// Release pipeline buffer and swap for the deinterleaved buffer
		((AkPipelineBuffer*)io_pIOBuffer)->ReleaseCachedBuffer();
		*io_pIOBuffer = *((AkAudioBuffer*)&DeinterleavedBuffer); //Just overwrite AkAudioBuffer part
	}
#endif
	return AK_Success;
}

#endif

void CAkResampler::TimeInputToOutput( AkUInt32 & io_uFrames )
{
	io_uFrames = (AkUInt32) ( io_uFrames / ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) + 0.5f ); // round to nearest
}

void CAkResampler::TimeOutputToInput( AkUInt32 & io_uFrames )
{
	io_uFrames = (AkUInt32) ( io_uFrames * ((AkReal32)m_InternalPitchState.uCurrentFrameSkip / FPMUL ) + 0.5f ); // round to nearest
}

void CAkResampler::ResetOffsets()
{
	m_InternalPitchState.uOutFrameOffset = 0;
	m_InternalPitchState.uInFrameOffset = 0;
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate conversion
//-----------------------------------------------------------------------------
AKRESULT CAkResampler::Term( )
{
#ifdef PERFORMANCE_BENCHMARK
	AkTChar szString[64];
	swprintf( szString, L"%f\n", m_fTotalTime/m_uNumberCalls );
	AKPLATFORM::OutputDebugMsg( szString );
	MONITOR_MSG( szString );
#endif 

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: SetPitch
// Desc: Change pitch shift (value provided in cents)
//-----------------------------------------------------------------------------
void CAkResampler::SetPitch( AkReal32 in_fPitchVal )
{
	// Clip pitch value to supported range
	if ( in_fPitchVal < AK_LE_MIN_PITCHSHIFTCENTS )
		in_fPitchVal = AK_LE_MIN_PITCHSHIFTCENTS;
	else if ( in_fPitchVal > AK_LE_MAX_PITCHSHIFTCENTS )
		in_fPitchVal = AK_LE_MAX_PITCHSHIFTCENTS;

	if ( m_bFirstSetPitch )
	{	
		// No interpolation required
		m_InternalPitchState.uCurrentFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_InternalPitchState.uTargetFrameSkip = m_InternalPitchState.uCurrentFrameSkip;				
		m_InternalPitchState.uInterpolationRampCount = PITCHRAMPLENGTH;
		m_fTargetPitchVal = in_fPitchVal;
		m_bFirstSetPitch = false;
	}

	if ( in_fPitchVal != m_fTargetPitchVal )
	{
		// New pitch interpolation is required
		m_InternalPitchState.uInterpolationRampCount = 0;	
		m_InternalPitchState.uTargetFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_fTargetPitchVal = in_fPitchVal;
	}

	if ( m_InternalPitchState.uCurrentFrameSkip != m_InternalPitchState.uTargetFrameSkip )
	{	
		// Route to appropriate pitch interpolating DSP
		m_PitchOperationMode = PitchOperatingMode_Interpolating;
	}
	else
	{
		// No interpolation required.
		// Bypass if effective resampling is within our fixed point precision
		if ( m_InternalPitchState.uCurrentFrameSkip == SINGLEFRAMEDISTANCE )
		{
			// Note: route the next execute dereference to appropriate DSP
			m_PitchOperationMode = PitchOperatingMode_Bypass;
		}
		else
		{
			// Done with the pitch change but need to resample at constant ratio
			m_PitchOperationMode = PitchOperatingMode_Fixed;
		}
	}
}

void CAkResampler::SetPitchForTimeSkip( AkReal32 in_fPitchVal )
{
	if ( m_bFirstSetPitch || in_fPitchVal != m_fTargetPitchVal )
	{
		// Clip pitch value to supported range
		if ( in_fPitchVal < AK_LE_MIN_PITCHSHIFTCENTS )
			in_fPitchVal = AK_LE_MIN_PITCHSHIFTCENTS;
		else if ( in_fPitchVal > AK_LE_MAX_PITCHSHIFTCENTS )
			in_fPitchVal = AK_LE_MAX_PITCHSHIFTCENTS;

		m_InternalPitchState.uCurrentFrameSkip = (AkUInt32) ( ( m_fSampleRateConvertRatio * powf(2.f, in_fPitchVal / 1200.f ) ) * FPMUL + 0.5 ) ; // round
		m_InternalPitchState.uTargetFrameSkip = m_InternalPitchState.uCurrentFrameSkip;
		m_InternalPitchState.uInterpolationRampCount = PITCHRAMPLENGTH;
		m_fTargetPitchVal = in_fPitchVal;
	}
}


