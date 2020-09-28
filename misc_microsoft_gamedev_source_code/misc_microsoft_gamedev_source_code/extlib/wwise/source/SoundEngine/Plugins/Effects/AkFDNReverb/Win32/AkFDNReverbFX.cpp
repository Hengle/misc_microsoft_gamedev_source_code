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
// AkFDNReverbFX.cpp
//
// FDN Reverb implementation.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFDNReverbFX.h"
#include <assert.h>
#include <math.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <stdlib.h>	// qsort

//#define MEMORYOUTPUT
#if defined(_DEBUG) && defined(MEMORYOUTPUT) || defined(OUTPUTPROCESSTIMER)
#include <stdio.h> 
#endif

// Note: Feedback matrix used (implicit) is HouseHolder matrix that maximizes echo density (no zero entries).
// This matrix recursion can be computed very efficiently in 2N operation using matrix properties. See example below.
// 4 x 4 using following values -2/N, 1-2/N
// -.5, .5, -.5 -.5
// -.5, -.5, .5 -.5
// -.5, -.5, -.5 .5
// .5, -.5, -.5 -.5
// Algorithm: 
// 1) Take the sum of all delay outputs d1in = (d1 + d2 + d3 + d4)
// 2) Multiply by -2/N -> d1in = -0.5(d1 + d2 + d3 + d4)
// 3) Add the full output of one delay line further to effectively change the coefficient that was wrong in the previous computation
// i.e. -> d1in = -0.5(d1 + d2 + d3 + d4) + d2 == -.5d1 + .5d2 -.5d3 -.5d4

static const AkReal32 DCFILTERCUTOFFFREQ = 10.f;
static const AkReal32 TWOPI = 6.2831853071f;
static const AkReal64 IIRCOEFCALCCONST = log(10.0)/4.0;
static const __m128 vMinusOne = {-1.f, -1.f, -1.f, -1.f};
static const int iSplat0 = _MM_SHUFFLE(0,0,0,0);

// FIXME: Win32 FDN reverb relies on contiguous channels!
#define LEFTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTLEFT * io_pBuffer->MaxFrames())
#define RIGHTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTRIGHT * io_pBuffer->MaxFrames())
#define CENTERCHANNELOFFSET			(AK_IDX_SETUP_5_CENTER * io_pBuffer->MaxFrames())
#define LFECHANNELOFFSET			(AK_IDX_SETUP_5_LFE * io_pBuffer->MaxFrames())
#define LEFTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARLEFT * io_pBuffer->MaxFrames())
#define RIGHTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARRIGHT * io_pBuffer->MaxFrames())

// Note: Matrix is constructed so that no column * -1 == another regardless of N (ensures proper decorrelation)
// Also sum of each column is == 0 as much as possible. This is not true for N = 4 for channels 5 and 6
// Otherwise constructed to be maximally different for each channel
static const __m128 vOutDecorrelationVectorA =		{ 1.f, -1.f,  1.f, -1.f };
static const __m128 vOutDecorrelationVectorB =		{ 1.f,  1.f, -1.f, -1.f };
static const __m128 vOutDecorrelationVectorC =		{-1.f,  1.f,  1.f, -1.f };
static const __m128 vOutDecorrelationVectorD =		{-1.f, -1.f, -1.f,  1.f };
static const __m128 vOutDecorrelationVectorE =		{ 1.f,  1.f, -1.f,  1.f };
static const __m128 vOutDecorrelationVectorF =		{ 1.f, -1.f, -1.f, -1.f };

//Group 0 1 2 3
//L =	A A A A
//R =	B B B B
//C =	C C C C
//LFE = B -B B -B
//LS =	D E -B B
//RS =	F -D -C -A


// Generic N channel routine use the following (explicit) table
static const __m128 vOutDecorrelationVector[6][4] =
{
	{ {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f}, {1.f,-1.f,1.f,-1.f} },
	{ {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f}, {1.f,1.f,-1.f,-1.f} },
	{ {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,1.f,1.f,-1.f} },
	{ {1.f,1.f,-1.f,-1.f}, {-1.f,-1.f,1.f,1.f}, {1.f,1.f,-1.f,-1.f}, {-1.f,-1.f,1.f,1.f} },
	{ {-1.f,-1.f,-1.f,1.f}, {1.f,1.f,-1.f,1.f}, {-1.f,-1.f,1.f,1.f}, {1.f,1.f,-1.f,-1.f} },
	{ {1.f,-1.f,-1.f,-1.f}, {1.f,1.f,-1.f,1.f}, {1.f,-1.f,-1.f,1.f}, {-1.f,1.f,-1.f,1.f} },
};

static int AkFDNQSortCompare(const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateMatrixReverbFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFX( ) );
}


// Constructor.
CAkFDNReverbFX::CAkFDNReverbFX()
{
	m_pSharedParams = NULL;
	AKPLATFORM::AkMemSet( &m_FXParams, 0, sizeof(m_FXParams) );
	m_fpPerformDSP = NULL;
	m_pfPreDelayStart = NULL;
	m_pfPreDelayRW = NULL;
	m_pfPreDelayEnd = NULL;
	AKPLATFORM::AkMemSet( m_pfDelayStart, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_pfDelayEnd, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_pfDelayRead, 0, MAXNUMDELAYS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_pfDelayWrite, 0, MAXNUMDELAYGROUPS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_uNominalDelayLength, 0, MAXNUMDELAYS*sizeof(AkUInt32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFB0, 0, MAXNUMDELAYS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFA1, 0, MAXNUMDELAYS*sizeof(AkReal32) );
	AKPLATFORM::AkMemSet( m_vIIRLPFMem, 0, MAXNUMDELAYS*sizeof(AkReal32) );
}

// Destructor.
CAkFDNReverbFX::~CAkFDNReverbFX()
{
	
}

// Initializes and allocate memory for the effect
AKRESULT CAkFDNReverbFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &	in_rFormat					// Required audio input format.
							   )
{
	m_uSampleRate = in_rFormat.uSampleRate;
	m_bIsSentMode = in_pFXCtx->IsSendModeEffect();
	m_pSharedParams = reinterpret_cast<CAkFDNReverbFXParams*>(in_pParams);
	m_pSharedParams->GetParams( &m_FXParams );

	if ( m_FXParams.uDelayLengthsMode == AKDELAYLENGTHSMODE_DEFAULT )
		SetDefaultDelayLengths( ); // Ignore delay parameters and use default values

	// Setup DSP function ptr for current audio format
	assert( (m_FXParams.uNumberOfDelays % 4 == 0) && (m_FXParams.uNumberOfDelays <= 16) );

	AkChannelMask uChannelMask = in_rFormat.GetChannelMask();
	if ( !m_FXParams.uProcessLFE )
		uChannelMask = uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY;
	m_uNumProcessedChannels = AK::GetNumChannels( uChannelMask );

	switch ( uChannelMask )
	{
	case AK_SPEAKER_SETUP_MONO:
		switch ( m_FXParams.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessMono16;
			break;
		}
		break;
	case AK_SPEAKER_SETUP_STEREO:
		switch ( m_FXParams.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessStereo16;
			break;
		}
		break;
	case AK_SPEAKER_SETUP_5:
		switch ( m_FXParams.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointZero16;
			break;
		}
		break;
	case AK_SPEAKER_SETUP_5POINT1:
		switch ( m_FXParams.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessFivePointOne16;
			break;
		}
		break;
	default:
		switch ( m_FXParams.uNumberOfDelays )
		{
		case 4:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN4;
			break;
		case 8:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN8;
			break;
		case 12:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN12;
			break;
		case 16:
			m_fpPerformDSP = &CAkFDNReverbFX::ProcessN16;
			break;
		}
		break;
	}

#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	AkUInt32 uTotalMemoryAllocated = 0;
#endif

	////////////////////// Allocate and setup pre-delay line ////////////////////

	m_uPreDelayLength = (AkUInt32) (m_FXParams.fPreDelay * m_uSampleRate);
	if ( m_uPreDelayLength > 0 )
	{
		m_pfPreDelayStart = (AkReal32*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * m_uPreDelayLength );
		if ( !m_pfPreDelayStart )
			return AK_InsufficientMemory;
		m_pfPreDelayRW = m_pfPreDelayStart;
		m_pfPreDelayEnd = m_pfPreDelayStart + m_uPreDelayLength;
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(AkReal32) * m_uPreDelayLength;
#endif
	}

	////////////////////// Convert delay line lengths ////////////////////

	// Ensure the values are prime numbers and sorted in increasing order
	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays; ++i )
	{
		m_uNominalDelayLength[i] = (AkUInt32)((m_FXParams.fDelayTime[i]/1000.f)*m_uSampleRate);
		MakePrimeNumber( m_uNominalDelayLength[i] );
	}
	qsort(m_uNominalDelayLength, m_FXParams.uNumberOfDelays, sizeof(AkUInt32), AkFDNQSortCompare);

	// Print out some useful information to Wwise debug window
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	AkReal32 fFreqDensity = 0.f;
	AkReal32 fEchoDensity = 0.f;
	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays; ++i )
	{
		fFreqDensity += m_uNominalDelayLength[i];
		fEchoDensity += (AkReal32)m_uSampleRate/m_uNominalDelayLength[i];
	}
	fFreqDensity /= m_uSampleRate;
	printf( "Matrix Reverb Statistics\n" );
	printf( "Frequency density: %f\n", fFreqDensity );
	printf( "Echo density: %f\n", fEchoDensity );
#endif

	//////////////////// Feedback LPF damping coefficients initialization ////////////////////
	ComputeIIRLPFCoefs();

	//////////////////// Tone correction LPF intialization ////////////////////
	ComputeFIRLPFCoefs();

	// Initialize cache value with intial ones 
	m_fCachedReverbTime = m_FXParams.fReverbTime;
	m_fCachedHFRatio = m_FXParams.fHFRatio;
	m_bPrevPreStop = false;
	m_uTailFramesRemaining = 0; // Only computed when entering pre-stop mode

	//////////////////// Initialize delay lines ////////////////////

	// 1) Allocate each delay line using total length to account for possible modulation 
	// 2) Setup read and write pointers according to delay lengths
	// Note: Delay lines are interleaved 4x4 to allow substantial memory and CPU optimizations
	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays/4; ++i )
	{
		AkUInt32 uMaxFramesPerGroup = m_uNominalDelayLength[i*4+3];
		AkUInt32 uInterleavedDelayLength = uMaxFramesPerGroup*4;
		m_pfDelayStart[i] = (AkReal32*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * uInterleavedDelayLength );
		if ( !m_pfDelayStart[i] )
			return AK_InsufficientMemory;
		m_pfDelayWrite[i] = m_pfDelayStart[i];
		m_pfDelayEnd[i] = m_pfDelayStart[i] + uInterleavedDelayLength;
		m_pfDelayRead[i*4] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4])*4);
		m_pfDelayRead[i*4+1] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4+1])*4+1);
		m_pfDelayRead[i*4+2] = m_pfDelayStart[i] + ((uMaxFramesPerGroup-m_uNominalDelayLength[i*4+2])*4+2);
		m_pfDelayRead[i*4+3] = m_pfDelayStart[i] + 3;
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(AkReal32) * uInterleavedDelayLength;
#endif
	}

	// Setup interpolation ramps for wet and dry parameters
	m_fCurrentDry = m_FXParams.fDryLevel;
	m_fCurrentWet = m_FXParams.fWetLevel;

	// Init DC filter
	m_fDCCoef = 1.f - (TWOPI * DCFILTERCUTOFFFREQ / m_uSampleRate);

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
	printf( "Total allocated memory: %d\n", uTotalMemoryAllocated );
#endif

#ifdef OUTPUTPROCESSTIMER
	m_fTotalExecutionTime = 0.f;
	m_uNumberExecutions = 0;
	AkInt64 iFreq;
	::QueryPerformanceFrequency( (LARGE_INTEGER*)&iFreq );
    m_fPerfCounterFreq = (AkReal32) ( iFreq / 1000 );
#endif

	return AK_Success;
}

// Terminates.
AKRESULT CAkFDNReverbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
#if defined OUTPUTPROCESSTIMER && !defined(_DEBUG)
	CHAR str[256];
	sprintf_s(str, "%f\n", m_fTotalExecutionTime/m_uNumberExecutions );
	OutputDebugString( str );
#endif

	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays/4; ++i )
	{
		if ( m_pfDelayStart[i] )
		{
			AK_PLUGIN_FREE( in_pAllocator, (AkReal32*)m_pfDelayStart[i] );
			m_pfDelayStart[i] = NULL;
		}
	}

	if ( m_pfPreDelayStart )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfPreDelayStart );
		m_pfPreDelayStart = NULL;
	}
		
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkFDNReverbFX::Reset( )
{
	// Note: No need to zero out temp buffer has they first get overwritten every Execute()

	// Reset pre-delay
	if ( m_pfPreDelayStart )
		AKPLATFORM::AkMemSet( m_pfPreDelayStart, 0, sizeof(AkReal32) * m_uPreDelayLength );

	// Reset FIR LPF filter memory
	m_fFIRLPFMem = 0.f;

	// Reset feedback delay line states
	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays/4; ++i )
	{
		// Reset IIR LPF filter memory
		m_vIIRLPFMem[i].m128_f32[0] = 0.f;
		m_vIIRLPFMem[i].m128_f32[1] = 0.f;
		m_vIIRLPFMem[i].m128_f32[2] = 0.f;
		m_vIIRLPFMem[i].m128_f32[3] = 0.f;
		// Reset delay line memory
		if ( m_pfDelayStart[i] )
			AKPLATFORM::AkMemSet( (AkReal32*)m_pfDelayStart[i], 0, sizeof(AkReal32) * (m_uNominalDelayLength[i*4+3]*4) );
	}

	// Reset DC filter
	m_fDCFwdMem = 0.f;
	m_fDCFbkMem = 0.f;

	return AK_Success;
}

// Effect info query.
AKRESULT CAkFDNReverbFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute FDN reverb DSP.
//-----------------------------------------------------------------------------
void CAkFDNReverbFX::Execute( AkAudioBuffer* io_pBuffer )
{
	// Silence LFE channel if LFE processing is disabled to avoid in-phase LFE doubling
	bool bSilenceLFE = m_bIsSentMode && io_pBuffer->HasLFE() && !m_FXParams.uProcessLFE;
	if ( bSilenceLFE )
		AKPLATFORM::AkMemSet( io_pBuffer->GetLFE() , 0, io_pBuffer->uValidFrames * sizeof(AkReal32) );

	if ( m_uNumProcessedChannels == 0 )
		return;

	m_pSharedParams->GetRTPCParams( &m_FXParams );

	// Update RTPC values as necessary
	bool bRecomputeTailLength = false;
	if ( m_FXParams.fReverbTime != m_fCachedReverbTime || m_FXParams.fHFRatio != m_fCachedHFRatio )
	{
		ComputeIIRLPFCoefs();
		ComputeFIRLPFCoefs();
		m_fCachedReverbTime = m_FXParams.fReverbTime;
		m_fCachedHFRatio = m_FXParams.fHFRatio;
		bRecomputeTailLength = true;
	}

	// Disable dry level for sent mode path (wet still active for flexibility)
	if ( m_bIsSentMode )
	{
		// Send mode has no dry path
		m_fCurrentDry = 0.f;
		m_FXParams.fDryLevel = 0.f;
	}

	// Determine how many samples will be processed and handle FX tail
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{	
		if ( bRecomputeTailLength || m_bPrevPreStop == false )
		{
			m_uTailFramesRemaining = (AkUInt32)( m_FXParams.fReverbTime * m_uSampleRate );
		}	
		AkUInt32 uNumTailFrames = PluginMin( m_uTailFramesRemaining, (AkUInt32)(io_pBuffer->MaxFrames()-io_pBuffer->uValidFrames) );
		m_uTailFramesRemaining -= uNumTailFrames; 
		ZeroInputTailFrames( io_pBuffer, uNumTailFrames );
		io_pBuffer->uValidFrames = io_pBuffer->uValidFrames + (AkUInt16)uNumTailFrames;
		if ( m_uTailFramesRemaining > 0 )
			io_pBuffer->eState = AK_DataReady;
	}

	m_bPrevPreStop = bPreStop;
	assert( io_pBuffer->uValidFrames <= io_pBuffer->MaxFrames() );

#ifdef WIN32
	AkUInt32 uFlushZeroMode = _MM_GET_FLUSH_ZERO_MODE(NOPARAMETER);
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

#ifdef OUTPUTPROCESSTIMER
	AkInt64 iBefore;
	::QueryPerformanceCounter( (LARGE_INTEGER*)&iBefore );
#endif

	// Dereference required perform method
	(this->*m_fpPerformDSP)( io_pBuffer );

#ifdef OUTPUTPROCESSTIMER
	AkInt64 iNow;
	::QueryPerformanceCounter( (LARGE_INTEGER*)&iNow );
	m_fTotalExecutionTime += ( iNow - iBefore ) / m_fPerfCounterFreq;
	m_uNumberExecutions++;
#endif


#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(uFlushZeroMode);
#endif

	m_fCurrentDry = m_FXParams.fDryLevel;
	m_fCurrentWet = m_FXParams.fWetLevel;
}

AkForceInline __m128 _mm_hadd_ps(__m128 i)
{   
	__m128 t = _mm_movehl_ps(i, i);
	i = _mm_add_ps(i, t);
	t = _mm_shuffle_ps(i, i, 0x55);
	i = _mm_add_ps(i, t);
	return i;
} 

AkForceInline __m128 _mm_rotateleft4_ps( __m128 a, __m128 b)
{   
	static const int iShuffle1 = _MM_SHUFFLE(0,0,3,3);
	static const int iShuffle2 = _MM_SHUFFLE(3,0,2,1);
	b = _mm_shuffle_ps(a, b, iShuffle1);
	b = _mm_shuffle_ps(a, b, iShuffle2);
	return b;
} 

#define _mm_madd_ps( __vIn1__, __vIn2__, __vIn3__ ) (_mm_add_ps( _mm_mul_ps( __vIn1__, __vIn2__ ), __vIn3__ ))
#define _mm_madd_ss( __vIn1__, __vIn2__, __vIn3__ ) (_mm_add_ss( _mm_mul_ss( __vIn1__, __vIn2__ ), __vIn3__ ))

#define GENERICPROCESSSETUP() \
	const __m128 vFIRLPFB0 = { m_fFIRLPFB0, 0.f, 0.f, 0.f }; \
	const __m128 vFIRLPFB1 = { m_fFIRLPFB1, 0.f, 0.f, 0.f }; \
	const __m128 vDCCoefs = { m_fDCCoef, 0.f, 0.f, 0.f }; \
	const AkReal32 fFeedbackConstant = -2.f/m_FXParams.uNumberOfDelays; \
	const __m128 vFeedbackConstant = {fFeedbackConstant, fFeedbackConstant, fFeedbackConstant, fFeedbackConstant}; \
	AkReal32 * AK_RESTRICT pInOut = io_pBuffer->GetChannel(0); \
	__m128 vCurDry = { m_fCurrentDry, 0.f, 0.f, 0.f }; \
	__m128 vCurWet = { m_fCurrentWet, 0.f, 0.f, 0.f }; \
	const AkReal32 fDryInc = (m_FXParams.fDryLevel - m_fCurrentDry) / io_pBuffer->MaxFrames(); \
	const AkReal32 fWetInc = (m_FXParams.fWetLevel - m_fCurrentWet) / io_pBuffer->MaxFrames(); \
	const __m128 vDryInc = { fDryInc, 0.f, 0.f, 0.f }; \
	const __m128 vWetInc = { fWetInc, 0.f, 0.f, 0.f }; \
	__m128 vFIRLPFMem = { m_fFIRLPFMem, 0.f, 0.f, 0.f }; \
	__m128 vDCxn1 = { m_fDCFwdMem, 0.f, 0.f, 0.f }; \
	__m128 vDCyn1 = { m_fDCFbkMem, 0.f, 0.f, 0.f }; \
	const AkReal32 * pfPreDelayStart = m_pfPreDelayStart; \
	AkReal32 * AK_RESTRICT pfPreDelayRW	= m_pfPreDelayRW; \
	const AkReal32 * pfPreDelayEnd = m_pfPreDelayEnd; \
	const bool bPreDelayProcessNeeded = m_pfPreDelayStart != NULL; 

#define DELAY4PROCESSSETUP() \
	const AkReal32* pfDelayStart = m_pfDelayStart[0]; \
	const AkReal32* pfDelayEnd = m_pfDelayEnd[0]; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	const __m128 vIIRLPFB0 = m_vIIRLPFB0[0]; \
	const __m128 vIIRLPFA1 = m_vIIRLPFA1[0]; \
	register __m128 vIIRLPFMem0 = m_vIIRLPFMem[0]; 

#define DELAY8PROCESSSETUP() \
	const AkReal32* pfDelayStart[2] = { m_pfDelayStart[0], m_pfDelayStart[1] }; \
	const AkReal32* pfDelayEnd[2] = { m_pfDelayEnd[0], m_pfDelayEnd[1] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	const __m128 vIIRLPFB0[2] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1] }; \
	const __m128 vIIRLPFA1[2] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1] }; \
	register __m128 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __m128 vIIRLPFMem1 = m_vIIRLPFMem[1]; 

#define DELAY12PROCESSSETUP() \
	const AkReal32* pfDelayStart[3] = { m_pfDelayStart[0], m_pfDelayStart[1], m_pfDelayStart[2] }; \
	const AkReal32* pfDelayEnd[3] = { m_pfDelayEnd[0], m_pfDelayEnd[1], m_pfDelayEnd[2] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayWrite2 = m_pfDelayWrite[2]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	register AkReal32* pfDelayRead8 = m_pfDelayRead[8]; \
	register AkReal32* pfDelayRead9 = m_pfDelayRead[9]; \
	register AkReal32* pfDelayRead10 = m_pfDelayRead[10]; \
	register AkReal32* pfDelayRead11 = m_pfDelayRead[11]; \
	const __m128 vIIRLPFB0[3] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2] }; \
	const __m128 vIIRLPFA1[3] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2] }; \
	register __m128 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __m128 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register __m128 vIIRLPFMem2 = m_vIIRLPFMem[2]; 

#define DELAY16PROCESSSETUP() \
	const AkReal32* pfDelayStart[4] = { m_pfDelayStart[0], m_pfDelayStart[1], m_pfDelayStart[2], m_pfDelayStart[3] }; \
	const AkReal32* pfDelayEnd[4] = { m_pfDelayEnd[0], m_pfDelayEnd[1], m_pfDelayEnd[2], m_pfDelayEnd[3] }; \
	register AkReal32* pfDelayWrite0 = m_pfDelayWrite[0]; \
	register AkReal32* pfDelayWrite1 = m_pfDelayWrite[1]; \
	register AkReal32* pfDelayWrite2 = m_pfDelayWrite[2]; \
	register AkReal32* pfDelayWrite3 = m_pfDelayWrite[3]; \
	register AkReal32* pfDelayRead0 = m_pfDelayRead[0]; \
	register AkReal32* pfDelayRead1 = m_pfDelayRead[1]; \
	register AkReal32* pfDelayRead2 = m_pfDelayRead[2]; \
	register AkReal32* pfDelayRead3 = m_pfDelayRead[3]; \
	register AkReal32* pfDelayRead4 = m_pfDelayRead[4]; \
	register AkReal32* pfDelayRead5 = m_pfDelayRead[5]; \
	register AkReal32* pfDelayRead6 = m_pfDelayRead[6]; \
	register AkReal32* pfDelayRead7 = m_pfDelayRead[7]; \
	register AkReal32* pfDelayRead8 = m_pfDelayRead[8]; \
	register AkReal32* pfDelayRead9 = m_pfDelayRead[9]; \
	register AkReal32* pfDelayRead10 = m_pfDelayRead[10]; \
	register AkReal32* pfDelayRead11 = m_pfDelayRead[11]; \
	register AkReal32* pfDelayRead12 = m_pfDelayRead[12]; \
	register AkReal32* pfDelayRead13 = m_pfDelayRead[13]; \
	register AkReal32* pfDelayRead14 = m_pfDelayRead[14]; \
	register AkReal32* pfDelayRead15 = m_pfDelayRead[15]; \
	const __m128 vIIRLPFB0[4] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2], m_vIIRLPFB0[3] }; \
	const __m128 vIIRLPFA1[4] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2], m_vIIRLPFA1[3] }; \
	register __m128 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __m128 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register __m128 vIIRLPFMem2 = m_vIIRLPFMem[2]; \
	register __m128 vIIRLPFMem3 = m_vIIRLPFMem[3]; 

#define GENERICPROCESSTEARDOWN() \
	_mm_store_ss( &m_fFIRLPFMem, vFIRLPFMem ); \
	_mm_store_ss( &m_fDCFwdMem, vDCxn1 );	\
	_mm_store_ss( &m_fDCFbkMem , vDCyn1 ); \
	m_pfPreDelayRW = pfPreDelayRW; 

#define DELAY16PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_vIIRLPFMem[2] = vIIRLPFMem2; \
	m_vIIRLPFMem[3] = vIIRLPFMem3; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayWrite[2] = pfDelayWrite2; \
	m_pfDelayWrite[3] = pfDelayWrite3; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; \
	m_pfDelayRead[8] = pfDelayRead8; \
	m_pfDelayRead[9] = pfDelayRead9; \
	m_pfDelayRead[10] = pfDelayRead10; \
	m_pfDelayRead[11] = pfDelayRead11; \
	m_pfDelayRead[12] = pfDelayRead12; \
	m_pfDelayRead[13] = pfDelayRead13; \
	m_pfDelayRead[14] = pfDelayRead14; \
	m_pfDelayRead[15] = pfDelayRead15; 

#define DELAY12PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_vIIRLPFMem[2] = vIIRLPFMem2; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayWrite[2] = pfDelayWrite2; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; \
	m_pfDelayRead[8] = pfDelayRead8; \
	m_pfDelayRead[9] = pfDelayRead9; \
	m_pfDelayRead[10] = pfDelayRead10; \
	m_pfDelayRead[11] = pfDelayRead11; 

#define DELAY8PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_vIIRLPFMem[1] = vIIRLPFMem1; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayWrite[1] = pfDelayWrite1; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; \
	m_pfDelayRead[4] = pfDelayRead4; \
	m_pfDelayRead[5] = pfDelayRead5; \
	m_pfDelayRead[6] = pfDelayRead6; \
	m_pfDelayRead[7] = pfDelayRead7; 

#define DELAY4PROCESSTEARDOWN() \
	m_vIIRLPFMem[0] = vIIRLPFMem0; \
	m_pfDelayWrite[0] = pfDelayWrite0; \
	m_pfDelayRead[0] = pfDelayRead0; \
	m_pfDelayRead[1] = pfDelayRead1; \
	m_pfDelayRead[2] = pfDelayRead2; \
	m_pfDelayRead[3] = pfDelayRead3; 

#define DAMPEDDELAYSGROUP0() \
	__m128 vDelayOut0 =  _mm_load_ss( pfDelayRead0 ); \
	__m128 vDelayOut1 =  _mm_load_ss( pfDelayRead1 ); \
	__m128 vDelayOut2 =  _mm_load_ss( pfDelayRead2 ); \
	__m128 vDelayOut3 =  _mm_load_ss( pfDelayRead3 ); \
	pfDelayRead0+=4; \
	if ( pfDelayRead0 >= pfDelayEnd[0] ) \
	pfDelayRead0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayRead1+=4; \
	if ( pfDelayRead1 >= pfDelayEnd[0] ) \
	pfDelayRead1 = (AkReal32*)pfDelayStart[0] + 1; \
	pfDelayRead2+=4; \
	if ( pfDelayRead2 >= pfDelayEnd[0] ) \
	pfDelayRead2 = (AkReal32*)pfDelayStart[0] + 2; \
	pfDelayRead3+=4; \
	if ( pfDelayRead3 >= pfDelayEnd[0] ) \
	pfDelayRead3 = (AkReal32*)pfDelayStart[0] + 3; \
	vDelayOut0 = _mm_unpacklo_ps( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = _mm_unpacklo_ps( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = _mm_movelh_ps( vDelayOut0, vDelayOut1 );	\
	__m128 vFbk = _mm_mul_ps( vIIRLPFA1[0], vIIRLPFMem0 ); \
	__m128 vDampedDelayOutputs0 = _mm_madd_ps( vDelayOut0, vIIRLPFB0[0], vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP0ALT() \
	__m128 vDelayOut0 =  _mm_load_ss( pfDelayRead0 ); \
	__m128 vDelayOut1 =  _mm_load_ss( pfDelayRead1 ); \
	__m128 vDelayOut2 =  _mm_load_ss( pfDelayRead2 ); \
	__m128 vDelayOut3 =  _mm_load_ss( pfDelayRead3 ); \
	pfDelayRead0+=4; \
	if ( pfDelayRead0 >= pfDelayEnd ) \
	pfDelayRead0 = (AkReal32*)pfDelayStart; \
	pfDelayRead1+=4; \
	if ( pfDelayRead1 >= pfDelayEnd ) \
	pfDelayRead1 = (AkReal32*)pfDelayStart + 1; \
	pfDelayRead2+=4; \
	if ( pfDelayRead2 >= pfDelayEnd ) \
	pfDelayRead2 = (AkReal32*)pfDelayStart + 2; \
	pfDelayRead3+=4; \
	if ( pfDelayRead3 >= pfDelayEnd ) \
	pfDelayRead3 = (AkReal32*)pfDelayStart + 3; \
	vDelayOut0 = _mm_unpacklo_ps( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = _mm_unpacklo_ps( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = _mm_movelh_ps( vDelayOut0, vDelayOut1 );	\
	__m128 vFbk = _mm_mul_ps( vIIRLPFA1, vIIRLPFMem0 ); \
	__m128 vDampedDelayOutputs0 = _mm_madd_ps( vDelayOut0, vIIRLPFB0, vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP1() \
	vDelayOut0 =  _mm_load_ss( pfDelayRead4 ); \
	vDelayOut1 =  _mm_load_ss( pfDelayRead5 ); \
	vDelayOut2 =  _mm_load_ss( pfDelayRead6 ); \
	vDelayOut3 =  _mm_load_ss( pfDelayRead7 ); \
	pfDelayRead4+=4; \
	if ( pfDelayRead4 >= pfDelayEnd[1] ) \
	pfDelayRead4 = (AkReal32*)pfDelayStart[1]; \
	pfDelayRead5+=4; \
	if ( pfDelayRead5 >= pfDelayEnd[1] ) \
	pfDelayRead5 = (AkReal32*)pfDelayStart[1] + 1; \
	pfDelayRead6+=4; \
	if ( pfDelayRead6 >= pfDelayEnd[1] ) \
	pfDelayRead6 = (AkReal32*)pfDelayStart[1] + 2; \
	pfDelayRead7+=4; \
	if ( pfDelayRead7 >= pfDelayEnd[1] ) \
	pfDelayRead7 = (AkReal32*)pfDelayStart[1] + 3; \
	vDelayOut0 = _mm_unpacklo_ps( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = _mm_unpacklo_ps( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = _mm_movelh_ps( vDelayOut0, vDelayOut1 );	\
	vFbk = _mm_mul_ps( vIIRLPFA1[1], vIIRLPFMem1 ); \
	__m128 vDampedDelayOutputs1 = _mm_madd_ps( vDelayOut0, vIIRLPFB0[1], vFbk ); \
	vIIRLPFMem1 = vDampedDelayOutputs1; 

#define DAMPEDDELAYSGROUP2() \
	vDelayOut0 =  _mm_load_ss( pfDelayRead8 ); \
	vDelayOut1 =  _mm_load_ss( pfDelayRead9 ); \
	vDelayOut2 =  _mm_load_ss( pfDelayRead10 ); \
	vDelayOut3 =  _mm_load_ss( pfDelayRead11 ); \
	pfDelayRead8+=4; \
	if ( pfDelayRead8 >= pfDelayEnd[2] ) \
	pfDelayRead8 = (AkReal32*)pfDelayStart[2]; \
	pfDelayRead9+=4; \
	if ( pfDelayRead9 >= pfDelayEnd[2] ) \
	pfDelayRead9 = (AkReal32*)pfDelayStart[2] + 1; \
	pfDelayRead10+=4; \
	if ( pfDelayRead10 >= pfDelayEnd[2] ) \
	pfDelayRead10 = (AkReal32*)pfDelayStart[2] + 2; \
	pfDelayRead11+=4; \
	if ( pfDelayRead11 >= pfDelayEnd[2] ) \
	pfDelayRead11 = (AkReal32*)pfDelayStart[2] + 3; \
	vDelayOut0 = _mm_unpacklo_ps( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = _mm_unpacklo_ps( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = _mm_movelh_ps( vDelayOut0, vDelayOut1 );	\
	vFbk = _mm_mul_ps( vIIRLPFA1[2], vIIRLPFMem2 ); \
	__m128 vDampedDelayOutputs2 = _mm_madd_ps( vDelayOut0, vIIRLPFB0[2], vFbk ); \
	vIIRLPFMem2 = vDampedDelayOutputs2; 

#define DAMPEDDELAYSGROUP3() \
	vDelayOut0 =  _mm_load_ss( pfDelayRead12 ); \
	vDelayOut1 =  _mm_load_ss( pfDelayRead13 ); \
	vDelayOut2 =  _mm_load_ss( pfDelayRead14 ); \
	vDelayOut3 =  _mm_load_ss( pfDelayRead15 ); \
	pfDelayRead12+=4; \
	if ( pfDelayRead12 >= pfDelayEnd[3] ) \
	pfDelayRead12 = (AkReal32*)pfDelayStart[3]; \
	pfDelayRead13+=4; \
	if ( pfDelayRead13 >= pfDelayEnd[3] ) \
	pfDelayRead13 = (AkReal32*)pfDelayStart[3] + 1; \
	pfDelayRead14+=4; \
	if ( pfDelayRead14 >= pfDelayEnd[3] ) \
	pfDelayRead14 = (AkReal32*)pfDelayStart[3] + 2; \
	pfDelayRead15+=4; \
	if ( pfDelayRead15 >= pfDelayEnd[3] ) \
	pfDelayRead15 = (AkReal32*)pfDelayStart[3] + 3; \
	vDelayOut0 = _mm_unpacklo_ps( vDelayOut0, vDelayOut1 ); \
	vDelayOut1 = _mm_unpacklo_ps( vDelayOut2, vDelayOut3 ); \
	vDelayOut0 = _mm_movelh_ps( vDelayOut0, vDelayOut1 );	\
	vFbk = _mm_mul_ps( vIIRLPFA1[3], vIIRLPFMem3 ); \
	__m128 vDampedDelayOutputs3 = _mm_madd_ps( vDelayOut0, vIIRLPFB0[3], vFbk ); \
	vIIRLPFMem3 = vDampedDelayOutputs3; 

#define DELAY16INJECTION() \
	vInputReinjection0 = _mm_add_ps( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = _mm_add_ps( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = _mm_add_ps( vInputReinjection2, vFIROut ); \
	vInputReinjection3 = _mm_add_ps( vInputReinjection3, vFIROut ); \
	_mm_store_ps( pfDelayWrite0, vInputReinjection0 ); \
	_mm_store_ps( pfDelayWrite1, vInputReinjection1 ); \
	_mm_store_ps( pfDelayWrite2, vInputReinjection2 ); \
	_mm_store_ps( pfDelayWrite3, vInputReinjection3 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; \
	pfDelayWrite2+=4; \
	if ( pfDelayWrite2 >= pfDelayEnd[2] ) \
	pfDelayWrite2 = (AkReal32*)pfDelayStart[2]; \
	pfDelayWrite3+=4; \
	if ( pfDelayWrite3 >= pfDelayEnd[3] ) \
	pfDelayWrite3 = (AkReal32*)pfDelayStart[3]; 

#define DELAY12INJECTION() \
	vInputReinjection0 = _mm_add_ps( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = _mm_add_ps( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = _mm_add_ps( vInputReinjection2, vFIROut ); \
	_mm_store_ps( pfDelayWrite0, vInputReinjection0 ); \
	_mm_store_ps( pfDelayWrite1, vInputReinjection1 ); \
	_mm_store_ps( pfDelayWrite2, vInputReinjection2 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; \
	pfDelayWrite2+=4; \
	if ( pfDelayWrite2 >= pfDelayEnd[2] ) \
	pfDelayWrite2 = (AkReal32*)pfDelayStart[2]; 

#define DELAY8INJECTION() \
	vInputReinjection0 = _mm_add_ps( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = _mm_add_ps( vInputReinjection1, vFIROut ); \
	_mm_store_ps( pfDelayWrite0, vInputReinjection0 ); \
	_mm_store_ps( pfDelayWrite1, vInputReinjection1 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; 

#define DELAY4INJECTION() \
	vInputReinjection0 = _mm_add_ps( vInputReinjection0, vFIROut ); \
	_mm_store_ps( pfDelayWrite0, vInputReinjection0 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart; 

#define SCALEINOUTN(__NUMCHANNELS__) \
	__m128 vIn[AK_VOICE_MAX_NUM_CHANNELS];\
	AkUInt32 index = 0;\
	do\
	{\
		vIn[index] = _mm_load_ss(&pInOut[index*io_pBuffer->MaxFrames()]); \
		vOut[index] = _mm_hadd_ps( vOut[index] ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	\
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = _mm_shuffle_ps( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = _mm_add_ss( vCurDry, vDryInc ); \
	vCurWet = _mm_add_ss( vCurWet, vWetInc ); \
	\
	index = 0;\
	do\
	{\
		vOut[index] = _mm_madd_ss( vIn[index], vCurDry, _mm_mul_ss( vOut[index], vCurWet ) ); \
		_mm_store_ss( &pInOut[index*io_pBuffer->MaxFrames()], vOut[index] ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	++pInOut;


#define SCALEINOUTFIVEPOINTONE() \
	__m128 vInL = _mm_load_ss(&pInOut[LEFTCHANNELOFFSET]); \
	__m128 vInR = _mm_load_ss(&pInOut[RIGHTCHANNELOFFSET]); \
	__m128 vInC = _mm_load_ss(&pInOut[CENTERCHANNELOFFSET]); \
	__m128 vInLFE = _mm_load_ss(&pInOut[LFECHANNELOFFSET]); \
	__m128 vInLS = _mm_load_ss(&pInOut[LEFTSURROUNDCHANNELOFFSET]); \
	__m128 vInRS = _mm_load_ss(&pInOut[RIGHTSURROUNDCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vOutC = _mm_hadd_ps( vOutC ); \
	vOutLFE = _mm_hadd_ps( vOutLFE ); \
	vOutLS = _mm_hadd_ps( vOutLS ); \
	vOutRS = _mm_hadd_ps( vOutRS ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = _mm_shuffle_ps( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = _mm_add_ss( vCurDry, vDryInc ); \
	vCurWet = _mm_add_ss( vCurWet, vWetInc ); \
	\
	vOutL = _mm_madd_ss( vInL, vCurDry, _mm_mul_ss( vOutL, vCurWet ) ); \
	vOutR = _mm_madd_ss( vInR, vCurDry, _mm_mul_ss( vOutR, vCurWet ) ); \
	vOutC = _mm_madd_ss( vInC, vCurDry, _mm_mul_ss( vOutC, vCurWet ) ); \
	vOutLFE = _mm_madd_ss( vInLFE, vCurDry, _mm_mul_ss( vOutLFE, vCurWet ) ); \
	vOutLS = _mm_madd_ss( vInLS, vCurDry, _mm_mul_ss( vOutLS, vCurWet ) ); \
	vOutRS = _mm_madd_ss( vInRS, vCurDry, _mm_mul_ss( vOutRS, vCurWet ) ); \
	_mm_store_ss( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	_mm_store_ss( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	_mm_store_ss( &pInOut[CENTERCHANNELOFFSET], vOutC ); \
	_mm_store_ss( &pInOut[LFECHANNELOFFSET], vOutLFE ); \
	_mm_store_ss( &pInOut[LEFTSURROUNDCHANNELOFFSET], vOutLS ); \
	_mm_store_ss( &pInOut[RIGHTSURROUNDCHANNELOFFSET], vOutRS ); \
	++pInOut; 

#define SCALEINOUTFIVEPOINTZERO() \
	__m128 vInL = _mm_load_ss(&pInOut[LEFTCHANNELOFFSET]); \
	__m128 vInR = _mm_load_ss(&pInOut[RIGHTCHANNELOFFSET]); \
	__m128 vInC = _mm_load_ss(&pInOut[CENTERCHANNELOFFSET]); \
	__m128 vInLS = _mm_load_ss(&pInOut[LEFTSURROUNDCHANNELOFFSET]); \
	__m128 vInRS = _mm_load_ss(&pInOut[RIGHTSURROUNDCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vOutC = _mm_hadd_ps( vOutC ); \
	vOutLS = _mm_hadd_ps( vOutLS ); \
	vOutRS = _mm_hadd_ps( vOutRS ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = _mm_shuffle_ps( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = _mm_add_ss( vCurDry, vDryInc ); \
	vCurWet = _mm_add_ss( vCurWet, vWetInc ); \
	\
	vOutL = _mm_madd_ss( vInL, vCurDry, _mm_mul_ss( vOutL, vCurWet ) ); \
	vOutR = _mm_madd_ss( vInR, vCurDry, _mm_mul_ss( vOutR, vCurWet ) ); \
	vOutC = _mm_madd_ss( vInC, vCurDry, _mm_mul_ss( vOutC, vCurWet ) ); \
	vOutLS = _mm_madd_ss( vInLS, vCurDry, _mm_mul_ss( vOutLS, vCurWet ) ); \
	vOutRS = _mm_madd_ss( vInRS, vCurDry, _mm_mul_ss( vOutRS, vCurWet ) ); \
	_mm_store_ss( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	_mm_store_ss( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	_mm_store_ss( &pInOut[CENTERCHANNELOFFSET], vOutC ); \
	_mm_store_ss( &pInOut[LEFTSURROUNDCHANNELOFFSET], vOutLS ); \
	_mm_store_ss( &pInOut[RIGHTSURROUNDCHANNELOFFSET], vOutRS ); \
	++pInOut; 

#define SCALEINOUTSTEREO() \
	__m128 vInL = _mm_load_ss(&pInOut[LEFTCHANNELOFFSET]); \
	__m128 vInR = _mm_load_ss(&pInOut[RIGHTCHANNELOFFSET]); \
	\
	vOutL = _mm_hadd_ps( vOutL ); \
	vOutR = _mm_hadd_ps( vOutR ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = _mm_shuffle_ps( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = _mm_add_ss( vCurDry, vDryInc ); \
	vCurWet = _mm_add_ss( vCurWet, vWetInc ); \
	\
	vOutL = _mm_madd_ss( vInL, vCurDry, _mm_mul_ss( vOutL, vCurWet ) ); \
	vOutR = _mm_madd_ss( vInR, vCurDry, _mm_mul_ss( vOutR, vCurWet ) ); \
	_mm_store_ss( &pInOut[LEFTCHANNELOFFSET], vOutL ); \
	_mm_store_ss( &pInOut[RIGHTCHANNELOFFSET], vOutR ); \
	++pInOut; 

#define SCALEINOUTMONO() \
	__m128 vIn = _mm_load_ss(pInOut); \
	\
	vOut = _mm_hadd_ps( vOut ); \
	vScaleFactor = _mm_hadd_ps( vScaleFactor ); \
	vScaleFactor = _mm_shuffle_ps( vScaleFactor, vScaleFactor, iSplat0); \
	\
	vCurDry = _mm_add_ss( vCurDry, vDryInc ); \
	vCurWet = _mm_add_ss( vCurWet, vWetInc ); \
	\
	vOut = _mm_madd_ss( vIn, vCurDry, _mm_mul_ss( vOut, vCurWet ) ); \
	_mm_store_ss( pInOut, vOut ); \
	++pInOut; 

#define COMPUTEFEEDBACK16() \
	vScaleFactor = _mm_mul_ps( vScaleFactor, vFeedbackConstant ); \
	__m128 vPreviousVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vCurrentVec = _mm_add_ps( vDampedDelayOutputs3, vScaleFactor ); \
	__m128 vInputReinjection3 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = _mm_add_ps( vDampedDelayOutputs2, vScaleFactor ); \
	__m128 vInputReinjection2 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = _mm_add_ps( vDampedDelayOutputs1, vScaleFactor ); \
	__m128 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vInputReinjection0 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); 

#define COMPUTEFEEDBACK12() \
	vScaleFactor = _mm_mul_ps( vScaleFactor, vFeedbackConstant ); \
	__m128 vPreviousVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vCurrentVec = _mm_add_ps( vDampedDelayOutputs2, vScaleFactor ); \
	__m128 vInputReinjection2 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = _mm_add_ps( vDampedDelayOutputs1, vScaleFactor ); \
	__m128 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vInputReinjection0 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); 

#define COMPUTEFEEDBACK8() \
	vScaleFactor = _mm_mul_ps( vScaleFactor, vFeedbackConstant ); \
	__m128 vPreviousVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vCurrentVec = _mm_add_ps( vDampedDelayOutputs1, vScaleFactor ); \
	__m128 vInputReinjection1 = _mm_rotateleft4_ps( vCurrentVec, vPreviousVec ); \
	__m128 vInputReinjection0 = _mm_rotateleft4_ps( vPreviousVec, vCurrentVec ); 

#define COMPUTEFEEDBACK4() \
	vScaleFactor = _mm_mul_ps( vScaleFactor, vFeedbackConstant ); \
	__m128 vPreviousVec = _mm_add_ps( vDampedDelayOutputs0, vScaleFactor ); \
	__m128 vInputReinjection0 = _mm_rotateleft4_ps( vPreviousVec, vPreviousVec ); 

#define PROCESSPREDELAY() \
	__m128 vPreDelayOut; \
	if ( bPreDelayProcessNeeded ) \
		{ \
		vPreDelayOut = _mm_load_ss( pfPreDelayRW ); \
		_mm_store_ss( pfPreDelayRW, vDCOut ); \
		++pfPreDelayRW; \
		if ( pfPreDelayRW == pfPreDelayEnd ) \
		pfPreDelayRW = (AkReal32*) pfPreDelayStart; \
		} \
	else \
	{ \
		vPreDelayOut = vDCOut; \
	} 

#define PROCESSTONECORRECTIONFILTER() \
	__m128 vFIROut = _mm_madd_ss( vFIRLPFB0, vPreDelayOut, _mm_mul_ss( vFIRLPFB1, vFIRLPFMem ) ); \
	vFIRLPFMem = vPreDelayOut; \
	vFIROut = _mm_shuffle_ps( vFIROut, vFIROut, iSplat0); 

#define PROCESSDCFILTER(__INPUT__) \
	__m128 vDCOut = _mm_sub_ss( _mm_madd_ss( vDCCoefs, vDCyn1, __INPUT__ ), vDCxn1 ); \
	vDCxn1 = __INPUT__; \
	vDCyn1 = vDCOut; 

#define MIXFIVEPOINTZERO() \
	__m128 vTmp = _mm_add_ss( vInL, vInR ); \
	__m128 vMixIn = _mm_add_ss( vInC, vInLS ); \
	vTmp = _mm_add_ss( vTmp, vInRS ); \
	vMixIn = _mm_add_ss( vMixIn, vTmp ); 

#define MIXFIVEPOINTONE() \
	__m128 vTmp = _mm_add_ss( vInL, vInR ); \
	__m128 vMixIn = _mm_add_ss( vInC, vInLFE ); \
	vTmp = _mm_add_ss( vTmp, vInLS ); \
	vMixIn = _mm_add_ss( vMixIn, vInRS ); \
	vMixIn = _mm_add_ss( vMixIn, vTmp ); 

#define MIXN(__NUMCHANNELS__) \
	__m128 vMixIn = vIn[0];\
	index = 1;\
	while ( index < __NUMCHANNELS__ )\
	{\
		vMixIn = _mm_add_ss( vMixIn, vIn[index] ); \
		++index;\
	}

// Original
void CAkFDNReverbFX::ProcessMono4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		__m128 vOut = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTMONO();
		COMPUTEFEEDBACK4();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOut = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );
		
		SCALEINOUTMONO();
		COMPUTEFEEDBACK8();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOut = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTMONO();
		COMPUTEFEEDBACK12();
		PROCESSDCFILTER( vIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessMono16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOut = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOut = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOut );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTMONO();
		COMPUTEFEEDBACK16();
		PROCESSDCFILTER( vIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK4();
		__m128 vMixIn = _mm_add_ss( vInL, vInR );	
		PROCESSDCFILTER( vMixIn );
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();
	
		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK8();
		__m128 vMixIn = _mm_add_ss( vInL, vInR );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK12();
		__m128 vMixIn = _mm_add_ss( vInL, vInR );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessStereo16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK16();
		__m128 vMixIn = _mm_add_ss( vInL, vInR );
		PROCESSDCFILTER( vMixIn );			
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK4();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();
	
		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );
	
		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK8();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK12();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );			
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointZero16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();		
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTFIVEPOINTZERO();
		COMPUTEFEEDBACK16();
		MIXFIVEPOINTZERO();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLFE = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;
		
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK4();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLFE = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );
	
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK8();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLFE = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );
		
		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK12();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );	
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessFivePointOne16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		__m128 vOutL = _mm_mul_ps( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__m128 vOutR = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutC = _mm_mul_ps( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__m128 vOutLFE = _mm_mul_ps( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__m128 vOutLS = _mm_mul_ps( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__m128 vOutRS = _mm_mul_ps( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = _mm_madd_ps( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = _mm_madd_ps( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLFE = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs3, vOutLFE );
		vOutLS = _mm_madd_ps( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = _mm_madd_ps( _mm_mul_ps( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTFIVEPOINTONE();
		COMPUTEFEEDBACK16();
		MIXFIVEPOINTONE();
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		AkUInt32 i = 0;
		__m128 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = _mm_mul_ps( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK4();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY4INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY4PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN8( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY8PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		__m128 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = _mm_mul_ps( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK8();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY8INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY8PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN12( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY12PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		__m128 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = _mm_mul_ps( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK12();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY12INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY12PROCESSTEARDOWN();
}

void CAkFDNReverbFX::ProcessN16( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY16PROCESSSETUP();

	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	const AkUInt32 uNumProcessedChannels = m_uNumProcessedChannels;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0();
		AkUInt32 i = 0;
		__m128 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = _mm_mul_ps( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__m128 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		i = 0;
		do 
		{
			vOut[i] = _mm_madd_ps( vOutDecorrelationVector[i][3], vDampedDelayOutputs3, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = _mm_add_ps( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTN( uNumProcessedChannels );
		COMPUTEFEEDBACK16();
		MIXN( uNumProcessedChannels );
		PROCESSDCFILTER( vMixIn );		
		PROCESSPREDELAY();
		PROCESSTONECORRECTIONFILTER();
		DELAY16INJECTION();

		--uFramesToProcess;
	}

	GENERICPROCESSTEARDOWN();
	DELAY16PROCESSTEARDOWN();
}

//////////////////// Feedback LPF damping coefficients initialization ////////////////////
void CAkFDNReverbFX::ComputeIIRLPFCoefs( )
{
	// Note: Coefficients are computed as double but run-time computations are float
	AkReal64 fSamplingPeriod = 1.0 / (AkReal64) m_uSampleRate;
	AkReal64 fReverbTimeRatio = 1.0 / (AkReal64) m_FXParams.fHFRatio;
	AkReal64 fReverbTimeRatioScale = 1.0 - (1.0/(fReverbTimeRatio*fReverbTimeRatio));

	// Note: For larger delay length values with small reverb time and high HFRatio, the system may become unstable.
	// Fix: Look for those potential instabilities and update fReverbTimeRatioScale (dependent on HFRatio) untill it leads
	// to a stable system. The effective HFRatio may not be exactly the same as the user selected. It needs to be the same
	// for all filter computations.
	AkUInt32 uMaxDelayTime = m_uNominalDelayLength[m_FXParams.uNumberOfDelays-1];
	// Compute A1 coefficient for maximum delay length (worst case)
	AkReal64 fDelayTimeWC = uMaxDelayTime * fSamplingPeriod;
	AkReal64 fB0WC = pow( 0.001, fDelayTimeWC / (AkReal64) m_FXParams.fReverbTime );
	AkReal64 fScaleFactorWC = log10(fB0WC)*IIRCOEFCALCCONST;
	AkReal64 fA1WC = (fScaleFactorWC*fReverbTimeRatioScale);
	if ( fA1WC > 1.0 )
	{
		// Compute new fReverbTimeRatioScale for worst case scenario to avoid unstability
		fReverbTimeRatioScale = 1.0 / fScaleFactorWC;
	}

	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays; ++i )
	{
		AkReal64 fDelayTime = m_uNominalDelayLength[i] * fSamplingPeriod;
		AkReal64 fB0 = pow( 0.001, fDelayTime / (AkReal64) m_FXParams.fReverbTime );
		assert( fB0 >= 0.f && fB0 < 1.f );
		AkReal64 fA1 = (log10(fB0)*IIRCOEFCALCCONST*fReverbTimeRatioScale);
		m_vIIRLPFB0[i/4].m128_f32[i%4] = (AkReal32)( fB0*(1.0-fA1) );
		m_vIIRLPFA1[i/4].m128_f32[i%4] =  (AkReal32)fA1;
	}
}

//////////////////// Tone correction LPF intialization ////////////////////
void CAkFDNReverbFX::ComputeFIRLPFCoefs( )
{
	AkReal64 fReverbTimeRatio = 1.0 / (AkReal64) m_FXParams.fHFRatio;
	AkReal64 fBeta = (1.0 - fReverbTimeRatio)/(1.0 + fReverbTimeRatio);
	AkReal64 fOneMinusBeta = (1 - fBeta);
	m_fFIRLPFB0 = (AkReal32) ( 1.0 / fOneMinusBeta );
	m_fFIRLPFB1 = (AkReal32) ( -fBeta / fOneMinusBeta );
}


//////////// Change an input integer value into its next prime value /////
void CAkFDNReverbFX::MakePrimeNumber( AkUInt32 & in_uIn )
{
	// First ensure its odd
	if ( (in_uIn & 1) == 0) 
		in_uIn++;	

	// Only need to compute up to square root (math theorem)
	AkInt32 iStop = (AkInt32) sqrt((AkReal64) in_uIn) + 1; 
	while ( true ) 
	{
		bool bFoundDivisor = false;
		for (AkInt32 i = 3; i < iStop; i+=2 )
		{
			if ( (in_uIn % i) == 0) 
			{
				// Can be divided by some number so not prime
				bFoundDivisor = true;
				break;
			}
		}

		if (!bFoundDivisor)
			break;		// Could not find dividors so its a prime number
		in_uIn += 2;	// Otherwise try the next odd number
	}
}

void CAkFDNReverbFX::ZeroInputTailFrames( AkAudioBuffer * io_pBuffer, AkUInt32 uNumTailFrames )
{
	// Zero out all channels.
	// Still need to zero out LFE channel even if it is not processed to make the data valid.
	AkUInt32 uZeroChannels = io_pBuffer->NumChannels();
	for ( AkUInt32 i = 0; i < uZeroChannels; ++i )
	{
		AKPLATFORM::AkMemSet( (io_pBuffer->GetChannel(i) + io_pBuffer->uValidFrames), 0, uNumTailFrames * sizeof(AkReal32) );
	}
}

void CAkFDNReverbFX::SetDefaultDelayLengths( )
{
	for ( AkUInt32 i = 0; i < m_FXParams.uNumberOfDelays; ++i )
	{
		m_FXParams.fDelayTime[i] = g_fDefaultDelayLengths[i]; 
	}
}

