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
#if defined(_DEBUG) && defined(MEMORYOUTPUT)
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
static const __vector4 vZero = {0.f,0.f, 0.f, 0.f};

// There is no other way to initialize static const __vector variable than to specify it in float...
static const __vector4 vMergeInOutShuffle = { /*0x00010203*/ 9.2557e-041, /*0x10111213*/ 2.86101317e-029, /*dummy*/ 0, /*dummy*/ 0 };
static const __vector4 vRotateShuffle = { /*0x04050607*/ 1.56368425e-036, /*0x08090A0B*/ 4.12387433e-034, /*0x0C0D0E0F*/ 1.08664755e-031, /*0x10111213*/ 2.86101317e-029 };
static const __vector4 vDelayInputShuffle = { /*0x00010203*/ 9.2557e-041, /*0x10111213*/ 2.86101317e-029, /*0x04050607*/ 1.56368425e-036, /*0x14151617*/ 7.52693405e-027 };
static const __vector4 vFloatOne = {1.f, 1.f, 1.f, 1.f};
static const __vector4 vMinusOne = {-1.f, -1.f, -1.f, -1.f};

// FIXME: Win32 FDN reverb relies on contiguous channels!
#define LEFTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTLEFT * io_pBuffer->MaxFrames() * sizeof(AkReal32) )
#define RIGHTCHANNELOFFSET			(AK_IDX_SETUP_5_FRONTRIGHT * io_pBuffer->MaxFrames() * sizeof(AkReal32) )
#define CENTERCHANNELOFFSET			(AK_IDX_SETUP_5_CENTER * io_pBuffer->MaxFrames() * sizeof(AkReal32))
#define LFECHANNELOFFSET			(AK_IDX_SETUP_5_LFE * io_pBuffer->MaxFrames() * sizeof(AkReal32) )
#define LEFTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARLEFT * io_pBuffer->MaxFrames() * sizeof(AkReal32))
#define RIGHTSURROUNDCHANNELOFFSET	(AK_IDX_SETUP_5_REARRIGHT * io_pBuffer->MaxFrames() * sizeof(AkReal32))
// Note: Matrix is constructed so that no column * -1 == another regardless of N (ensures proper decorrelation)
// Also sum of each column is == 0 as much as possible. This is not true for N = 4 for channels 5 and 6
// Otherwise constructed to be maximally different for each channel
static const __vector4 vOutDecorrelationVectorA =		{ 1.f, -1.f,  1.f, -1.f };
static const __vector4 vOutDecorrelationVectorB =		{ 1.f,  1.f, -1.f, -1.f };
static const __vector4 vOutDecorrelationVectorC =		{-1.f,  1.f,  1.f, -1.f };
static const __vector4 vOutDecorrelationVectorD =		{-1.f, -1.f, -1.f,  1.f };
static const __vector4 vOutDecorrelationVectorE =		{ 1.f,  1.f, -1.f,  1.f };
static const __vector4 vOutDecorrelationVectorF =		{ 1.f, -1.f, -1.f, -1.f };

//Group 0 1 2 3
//L =	A A A A
//R =	B B B B
//C =	C C C C
//LFE = B -B B -B
//LS =	D E -B B
//RS =	F -D -C -A


// Generic N channel routine use the following (explicit) table
static const __vector4 vOutDecorrelationVector[6][4] =
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

	return AK_Success;
}

// Terminates.
AKRESULT CAkFDNReverbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
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
		m_vIIRLPFMem[i].v[0] = 0.f;
		m_vIIRLPFMem[i].v[1] = 0.f;
		m_vIIRLPFMem[i].v[2] = 0.f;
		m_vIIRLPFMem[i].v[3] = 0.f;
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
void CAkFDNReverbFX::Execute( AkAudioBuffer* io_pBuffer )		// Input buffer interface.
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

	// Dereference required perform method
	(this->*m_fpPerformDSP)( io_pBuffer );

	m_fCurrentDry = m_FXParams.fDryLevel;
	m_fCurrentWet = m_FXParams.fWetLevel;
}

#define GENERICPROCESSSETUP() \
	const __vector4 vFIRLPFB0 = { m_fFIRLPFB0, 0.f, 0.f, 0.f }; \
	const __vector4 vFIRLPFB1 = { m_fFIRLPFB1, 0.f, 0.f, 0.f }; \
	const __vector4 vDCCoefs = { m_fDCCoef, 0.f, 0.f, 0.f }; \
	const AkReal32 fFeedbackConstant = -2.f/m_FXParams.uNumberOfDelays; \
	const __vector4 vFeedbackConstant = {fFeedbackConstant, fFeedbackConstant, fFeedbackConstant, fFeedbackConstant}; \
	AkReal32 * AK_RESTRICT pInOut = io_pBuffer->GetChannel(0); \
	__vector4 vCurRamp = { m_fCurrentDry, m_fCurrentWet, 0.f, 0.f }; \
	const AkReal32 fDryInc = (m_FXParams.fDryLevel - m_fCurrentDry) / io_pBuffer->MaxFrames(); \
	const AkReal32 fWetInc = (m_FXParams.fWetLevel - m_fCurrentWet) / io_pBuffer->MaxFrames(); \
	const __vector4 vRampInc = { fDryInc, fWetInc, 0.f, 0.f }; \
	__vector4 vFIRLPFMem = { m_fFIRLPFMem, 0.f, 0.f, 0.f }; \
	__vector4 vDCxn1 = { m_fDCFwdMem, 0.f, 0.f, 0.f }; \
	__vector4 vDCyn1 = { m_fDCFbkMem, 0.f, 0.f, 0.f }; \
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
	const __vector4 vIIRLPFB0 = m_vIIRLPFB0[0]; \
	const __vector4 vIIRLPFA1 = m_vIIRLPFA1[0]; \
	register __vector4 vIIRLPFMem0 = m_vIIRLPFMem[0]; 

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
	const __vector4 vIIRLPFB0[2] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1] }; \
	const __vector4 vIIRLPFA1[2] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1] }; \
	register __vector4 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __vector4 vIIRLPFMem1 = m_vIIRLPFMem[1]; 

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
	const __vector4 vIIRLPFB0[3] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2] }; \
	const __vector4 vIIRLPFA1[3] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2] }; \
	register __vector4 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __vector4 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register __vector4 vIIRLPFMem2 = m_vIIRLPFMem[2]; 

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
	const __vector4 vIIRLPFB0[4] = { m_vIIRLPFB0[0], m_vIIRLPFB0[1], m_vIIRLPFB0[2], m_vIIRLPFB0[3] }; \
	const __vector4 vIIRLPFA1[4] = { m_vIIRLPFA1[0], m_vIIRLPFA1[1], m_vIIRLPFA1[2], m_vIIRLPFA1[3] }; \
	register __vector4 vIIRLPFMem0 = m_vIIRLPFMem[0]; \
	register __vector4 vIIRLPFMem1 = m_vIIRLPFMem[1]; \
	register __vector4 vIIRLPFMem2 = m_vIIRLPFMem[2]; \
	register __vector4 vIIRLPFMem3 = m_vIIRLPFMem[3]; 

#define GENERICPROCESSTEARDOWN() \
	m_fFIRLPFMem = vFIRLPFMem.x; \
	m_fDCFwdMem = vDCxn1.x;	\
	m_fDCFbkMem = vDCyn1.x; \
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
	__vector4 vDelayOut0 =  __lvlx( pfDelayRead0, 0 ); \
	__vector4 vDelayOut1 =  __lvlx( pfDelayRead1, 0 ); \
	__vector4 vDelayOut2 =  __lvlx( pfDelayRead2, 0 ); \
	__vector4 vDelayOut3 =  __lvlx( pfDelayRead3, 0 ); \
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
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut2, vDelayInputShuffle ); \
	vDelayOut1 = __vperm( vDelayOut1, vDelayOut3, vDelayInputShuffle ); \
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut1, vDelayInputShuffle );	\
	__vector4 vFbk = __vmulfp( vIIRLPFA1[0], vIIRLPFMem0 ); \
	__vector4 vDampedDelayOutputs0 = __vmaddfp( vDelayOut0, vIIRLPFB0[0], vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP0ALT() \
	__vector4 vDelayOut0 =  __lvlx( pfDelayRead0, 0 ); \
	__vector4 vDelayOut1 =  __lvlx( pfDelayRead1, 0 ); \
	__vector4 vDelayOut2 =  __lvlx( pfDelayRead2, 0 ); \
	__vector4 vDelayOut3 =  __lvlx( pfDelayRead3, 0 ); \
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
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut2, vDelayInputShuffle ); \
	vDelayOut1 = __vperm( vDelayOut1, vDelayOut3, vDelayInputShuffle ); \
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut1, vDelayInputShuffle ); \
	__vector4 vFbk = __vmulfp( vIIRLPFA1, vIIRLPFMem0 ); \
	__vector4 vDampedDelayOutputs0 = __vmaddfp( vDelayOut0, vIIRLPFB0, vFbk ); \
	vIIRLPFMem0 = vDampedDelayOutputs0; 

#define DAMPEDDELAYSGROUP1() \
	vDelayOut0 =  __lvlx( pfDelayRead4, 0 ); \
	vDelayOut1 =  __lvlx( pfDelayRead5, 0 ); \
	vDelayOut2 =  __lvlx( pfDelayRead6, 0 ); \
	vDelayOut3 =  __lvlx( pfDelayRead7, 0 ); \
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
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut2, vDelayInputShuffle ); \
	vDelayOut1 = __vperm( vDelayOut1, vDelayOut3, vDelayInputShuffle ); \
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut1, vDelayInputShuffle );	\
	vFbk = __vmulfp( vIIRLPFA1[1], vIIRLPFMem1 ); \
	__vector4 vDampedDelayOutputs1 = __vmaddfp( vDelayOut0, vIIRLPFB0[1], vFbk ); \
	vIIRLPFMem1 = vDampedDelayOutputs1; 

#define DAMPEDDELAYSGROUP2() \
	vDelayOut0 =  __lvlx( pfDelayRead8, 0 ); \
	vDelayOut1 =  __lvlx( pfDelayRead9, 0 ); \
	vDelayOut2 =  __lvlx( pfDelayRead10, 0 ); \
	vDelayOut3 =  __lvlx( pfDelayRead11, 0 ); \
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
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut2, vDelayInputShuffle ); \
	vDelayOut1 = __vperm( vDelayOut1, vDelayOut3, vDelayInputShuffle ); \
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut1, vDelayInputShuffle );	\
	vFbk = __vmulfp( vIIRLPFA1[2], vIIRLPFMem2 ); \
	__vector4 vDampedDelayOutputs2 = __vmaddfp( vDelayOut0, vIIRLPFB0[2], vFbk ); \
	vIIRLPFMem2 = vDampedDelayOutputs2; 

#define DAMPEDDELAYSGROUP3() \
	vDelayOut0 =  __lvlx( pfDelayRead12, 0 ); \
	vDelayOut1 =  __lvlx( pfDelayRead13, 0 ); \
	vDelayOut2 =  __lvlx( pfDelayRead14, 0 ); \
	vDelayOut3 =  __lvlx( pfDelayRead15, 0 ); \
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
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut2, vDelayInputShuffle ); \
	vDelayOut1 = __vperm( vDelayOut1, vDelayOut3, vDelayInputShuffle ); \
	vDelayOut0 = __vperm( vDelayOut0, vDelayOut1, vDelayInputShuffle );	\
	vFbk = __vmulfp( vIIRLPFA1[3], vIIRLPFMem3 ); \
	__vector4 vDampedDelayOutputs3 = __vmaddfp( vDelayOut0, vIIRLPFB0[3], vFbk ); \
	vIIRLPFMem3 = vDampedDelayOutputs3; 

#define DELAY16INJECTION() \
	vInputReinjection0 = __vaddfp( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = __vaddfp( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = __vaddfp( vInputReinjection2, vFIROut ); \
	vInputReinjection3 = __vaddfp( vInputReinjection3, vFIROut ); \
	__stvx( vInputReinjection0, pfDelayWrite0, 0 ); \
	__stvx( vInputReinjection1, pfDelayWrite1, 0 ); \
	__stvx( vInputReinjection2, pfDelayWrite2, 0 ); \
	__stvx( vInputReinjection3, pfDelayWrite3, 0 ); \
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
	vInputReinjection0 = __vaddfp( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = __vaddfp( vInputReinjection1, vFIROut ); \
	vInputReinjection2 = __vaddfp( vInputReinjection2, vFIROut ); \
	__stvx( vInputReinjection0, pfDelayWrite0, 0 ); \
	__stvx( vInputReinjection1, pfDelayWrite1, 0 ); \
	__stvx( vInputReinjection2, pfDelayWrite2, 0 ); \
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
	vInputReinjection0 = __vaddfp( vInputReinjection0, vFIROut ); \
	vInputReinjection1 = __vaddfp( vInputReinjection1, vFIROut ); \
	__stvx( vInputReinjection0, pfDelayWrite0, 0 ); \
	__stvx( vInputReinjection1, pfDelayWrite1, 0 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd[0] ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart[0]; \
	pfDelayWrite1+=4; \
	if ( pfDelayWrite1 >= pfDelayEnd[1] ) \
	pfDelayWrite1 = (AkReal32*)pfDelayStart[1]; 

#define DELAY4INJECTION() \
	vInputReinjection0 = __vaddfp( vInputReinjection0, vFIROut ); \
	__stvx( vInputReinjection0, pfDelayWrite0, 0 ); \
	pfDelayWrite0+=4; \
	if ( pfDelayWrite0 >= pfDelayEnd ) \
	pfDelayWrite0 = (AkReal32*)pfDelayStart; 

#define SCALEINOUTN(__NUMCHANNELS__) \
	__vector4 vIn[AK_VOICE_MAX_NUM_CHANNELS];\
	AkUInt32 index = 0;\
	do\
	{\
		vIn[index] = __lvlx(pInOut, index*io_pBuffer->MaxFrames()*sizeof(AkReal32) ); \
		vOut[index] = __vmsum4fp( vOut[index], vFloatOne ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	vScaleFactor = __vmsum4fp( vScaleFactor, vFloatOne );	\
	\
	vCurRamp = __vaddfp( vCurRamp, vRampInc ); \
	\
	index = 0;\
	do\
	{\
		vOut[index] = __vperm( vIn[index], vOut[index], vMergeInOutShuffle ); \
		vOut[index] = __vmsum3fp( vOut[index], vCurRamp ); \
		__stvewx( vOut[index], pInOut, index*io_pBuffer->MaxFrames()*sizeof(AkReal32) ); \
		++index;\
	}\
	while ( index < __NUMCHANNELS__ );\
	++pInOut;
	
#define SCALEINOUTFIVEPOINTONE() \
	__vector4 vInL = __lvlx(pInOut, LEFTCHANNELOFFSET); \
	__vector4 vInR = __lvlx(pInOut, RIGHTCHANNELOFFSET); \
	__vector4 vInC = __lvlx(pInOut, CENTERCHANNELOFFSET); \
	__vector4 vInLFE = __lvlx(pInOut, LFECHANNELOFFSET); \
	__vector4 vInLS = __lvlx(pInOut, LEFTSURROUNDCHANNELOFFSET); \
	__vector4 vInRS = __lvlx(pInOut, RIGHTSURROUNDCHANNELOFFSET); \
	\
	vOutL = __vmsum4fp( vOutL, vFloatOne ); \
	vOutR = __vmsum4fp( vOutR, vFloatOne ); \
	vOutC = __vmsum4fp( vOutC, vFloatOne ); \
	vOutLFE = __vmsum4fp( vOutLFE, vFloatOne ); \
	vOutLS = __vmsum4fp( vOutLS, vFloatOne ); \
	vOutRS = __vmsum4fp( vOutRS, vFloatOne ); \
	vScaleFactor = __vmsum4fp( vScaleFactor, vFloatOne ); \
	\
	vCurRamp = __vaddfp( vCurRamp, vRampInc ); \
	\
	vOutL = __vperm( vInL, vOutL, vMergeInOutShuffle ); \
	vOutR = __vperm( vInR, vOutR, vMergeInOutShuffle ); \
	vOutC = __vperm( vInC, vOutC, vMergeInOutShuffle ); \
	vOutLFE = __vperm( vInLFE, vOutLFE, vMergeInOutShuffle ); \
	vOutLS = __vperm( vInLS, vOutLS, vMergeInOutShuffle ); \
	vOutRS = __vperm( vInRS, vOutRS, vMergeInOutShuffle ); \
	vOutL = __vmsum3fp( vOutL, vCurRamp ); \
	vOutR = __vmsum3fp( vOutR, vCurRamp ); \
	vOutC = __vmsum3fp( vOutC, vCurRamp ); \
	vOutLFE = __vmsum3fp( vOutLFE, vCurRamp ); \
	vOutLS = __vmsum3fp( vOutLS, vCurRamp ); \
	vOutRS = __vmsum3fp( vOutRS, vCurRamp ); \
	__stvewx( vOutL, pInOut, LEFTCHANNELOFFSET ); \
	__stvewx( vOutR, pInOut, RIGHTCHANNELOFFSET ); \
	__stvewx( vOutC, pInOut, CENTERCHANNELOFFSET ); \
	__stvewx( vOutLFE, pInOut, LFECHANNELOFFSET ); \
	__stvewx( vOutLS, pInOut, LEFTSURROUNDCHANNELOFFSET ); \
	__stvewx( vOutRS, pInOut, RIGHTSURROUNDCHANNELOFFSET ); \
	++pInOut; 

#define SCALEINOUTFIVEPOINTZERO() \
	__vector4 vInL = __lvlx(pInOut, LEFTCHANNELOFFSET); \
	__vector4 vInR = __lvlx(pInOut, RIGHTCHANNELOFFSET); \
	__vector4 vInC = __lvlx(pInOut, CENTERCHANNELOFFSET); \
	__vector4 vInLS = __lvlx(pInOut, LEFTSURROUNDCHANNELOFFSET); \
	__vector4 vInRS = __lvlx(pInOut, RIGHTSURROUNDCHANNELOFFSET); \
	\
	vOutL = __vmsum4fp( vOutL, vFloatOne ); \
	vOutR = __vmsum4fp( vOutR, vFloatOne ); \
	vOutC = __vmsum4fp( vOutC, vFloatOne ); \
	vOutLS = __vmsum4fp( vOutLS, vFloatOne ); \
	vOutRS = __vmsum4fp( vOutRS, vFloatOne ); \
	vScaleFactor = __vmsum4fp( vScaleFactor, vFloatOne ); \
	\
	vCurRamp = __vaddfp( vCurRamp, vRampInc ); \
	\
	vOutL = __vperm( vInL, vOutL, vMergeInOutShuffle ); \
	vOutR = __vperm( vInR, vOutR, vMergeInOutShuffle ); \
	vOutC = __vperm( vInC, vOutC, vMergeInOutShuffle ); \
	vOutLS = __vperm( vInLS, vOutLS, vMergeInOutShuffle ); \
	vOutRS = __vperm( vInRS, vOutRS, vMergeInOutShuffle ); \
	vOutL = __vmsum3fp( vOutL, vCurRamp ); \
	vOutR = __vmsum3fp( vOutR, vCurRamp ); \
	vOutC = __vmsum3fp( vOutC, vCurRamp ); \
	vOutLS = __vmsum3fp( vOutLS, vCurRamp ); \
	vOutRS = __vmsum3fp( vOutRS, vCurRamp ); \
	__stvewx( vOutL, pInOut, LEFTCHANNELOFFSET );  \
	__stvewx( vOutR, pInOut, RIGHTCHANNELOFFSET ); \
	__stvewx( vOutC, pInOut, CENTERCHANNELOFFSET ); \
	__stvewx( vOutLS, pInOut, LEFTSURROUNDCHANNELOFFSET ); \
	__stvewx( vOutRS, pInOut, RIGHTSURROUNDCHANNELOFFSET ); \
	++pInOut; 

#define SCALEINOUTSTEREO() \
	__vector4 vInL = __lvlx(pInOut, LEFTCHANNELOFFSET); \
	__vector4 vInR = __lvlx(pInOut, RIGHTCHANNELOFFSET); \
	\
	vOutL = __vmsum4fp( vOutL, vFloatOne ); \
	vOutR = __vmsum4fp( vOutR, vFloatOne ); \
	vScaleFactor = __vmsum4fp( vScaleFactor, vFloatOne ); \
	\
	vCurRamp = __vaddfp( vCurRamp, vRampInc ); \
	\
	vOutL = __vperm( vInL, vOutL, vMergeInOutShuffle ); \
	vOutR = __vperm( vInR, vOutR, vMergeInOutShuffle ); \
	vOutL = __vmsum3fp( vOutL, vCurRamp ); \
	vOutR = __vmsum3fp( vOutR, vCurRamp ); \
	__stvewx( vOutL, pInOut, LEFTCHANNELOFFSET ); \
	__stvewx( vOutR, pInOut, RIGHTCHANNELOFFSET ); \
	++pInOut; 

#define SCALEINOUTMONO() \
	__vector4 vIn = __lvlx(pInOut, 0); \
	\
	vOut = __vmsum4fp( vOut, vFloatOne ); \
	vScaleFactor = __vmsum4fp( vScaleFactor, vFloatOne ); \
	\
	vCurRamp = __vaddfp( vCurRamp, vRampInc ); \
	\
	vOut = __vperm( vIn, vOut, vMergeInOutShuffle ); \
	vOut = __vmsum3fp( vOut, vCurRamp ); \
	__stvewx( vOut, pInOut, 0 ); \
	++pInOut; 

#define COMPUTEFEEDBACK16() \
	vScaleFactor = __vmulfp( vScaleFactor, vFeedbackConstant ); \
	__vector4 vPreviousVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vCurrentVec = __vaddfp( vDampedDelayOutputs3, vScaleFactor ); \
	__vector4 vInputReinjection3 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = __vaddfp( vDampedDelayOutputs2, vScaleFactor ); \
	__vector4 vInputReinjection2 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = __vaddfp( vDampedDelayOutputs1, vScaleFactor ); \
	__vector4 vInputReinjection1 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vInputReinjection0 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); 

#define COMPUTEFEEDBACK12() \
	vScaleFactor = __vmulfp( vScaleFactor, vFeedbackConstant ); \
	__vector4 vPreviousVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vCurrentVec = __vaddfp( vDampedDelayOutputs2, vScaleFactor ); \
	__vector4 vInputReinjection2 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = __vaddfp( vDampedDelayOutputs1, vScaleFactor ); \
	__vector4 vInputReinjection1 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	vPreviousVec = vCurrentVec; \
	vCurrentVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vInputReinjection0 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); 

#define COMPUTEFEEDBACK8() \
	vScaleFactor = __vmulfp( vScaleFactor, vFeedbackConstant ); \
	__vector4 vPreviousVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vCurrentVec = __vaddfp( vDampedDelayOutputs1, vScaleFactor ); \
	__vector4 vInputReinjection1 = __vperm( vCurrentVec, vPreviousVec, vRotateShuffle ); \
	__vector4 vInputReinjection0 = __vperm( vPreviousVec, vCurrentVec, vRotateShuffle ); 

#define COMPUTEFEEDBACK4() \
	vScaleFactor = __vmulfp( vScaleFactor, vFeedbackConstant ); \
	__vector4 vPreviousVec = __vaddfp( vDampedDelayOutputs0, vScaleFactor ); \
	__vector4 vInputReinjection0 = __vperm( vPreviousVec, vPreviousVec, vRotateShuffle ); 

#define PROCESSPREDELAY() \
	__vector4 vPreDelayOut; \
	if ( bPreDelayProcessNeeded ) \
		{ \
		vPreDelayOut = __lvlx( pfPreDelayRW, 0 ); \
		vDCOut = __vspltw( vDCOut, 0 ); \
		__stvewx( vDCOut, pfPreDelayRW, 0 ); \
		++pfPreDelayRW; \
		if ( pfPreDelayRW == pfPreDelayEnd ) \
		pfPreDelayRW = (AkReal32*) pfPreDelayStart; \
		} \
	else \
	{ \
		vPreDelayOut = vDCOut; \
	} 

#define PROCESSTONECORRECTIONFILTER() \
	__vector4 vFIROut = __vmaddfp( vFIRLPFB0, vPreDelayOut, __vmulfp( vFIRLPFB1, vFIRLPFMem ) ); \
	vFIRLPFMem = vPreDelayOut; \
	vFIROut = __vspltw( vFIROut, 0); 

#define PROCESSDCFILTER(__INPUT__) \
	__vector4 vDCOut = __vsubfp( __vmaddfp( vDCCoefs, vDCyn1, __INPUT__ ), vDCxn1 ); \
	vDCxn1 = __INPUT__; \
	vDCyn1 = vDCOut; 

#define MIXFIVEPOINTZERO() \
	__vector4 vTmp = __vaddfp( vInL, vInR ); \
	__vector4 vMixIn = __vaddfp( vInC, vInLS ); \
	vTmp = __vaddfp( vTmp, vInRS ); \
	vMixIn = __vaddfp( vMixIn, vTmp ); 

#define MIXFIVEPOINTONE() \
	__vector4 vTmp = __vaddfp( vInL, vInR ); \
	__vector4 vMixIn = __vaddfp( vInC, vInLFE ); \
	vTmp = __vaddfp( vTmp, vInLS ); \
	vMixIn = __vaddfp( vMixIn, vInRS ); \
	vMixIn = __vaddfp( vMixIn, vTmp ); 
		
#define MIXN(__NUMCHANNELS__) \
	__vector4 vMixIn = vIn[0];\
	index = 1;\
	while ( index < __NUMCHANNELS__ ) \
	{\
		vMixIn = __vaddfp( vMixIn, vIn[index] ); \
		++index;\
	}
	
void CAkFDNReverbFX::ProcessMono4( AkAudioBuffer * io_pBuffer )
{
	GENERICPROCESSSETUP();
	DELAY4PROCESSSETUP();
	
	AkUInt32 uFramesToProcess = io_pBuffer->uValidFrames;
	while(uFramesToProcess)
	{
		DAMPEDDELAYSGROUP0ALT();
		__vector4 vOut = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

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
		__vector4 vOut = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );
		
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
		__vector4 vOut = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

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
		__vector4 vOut = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOut = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOut );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs3 );

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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK4();
		__vector4 vMixIn = __vaddfp( vInL, vInR );	
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK8();
		__vector4 vMixIn = __vaddfp( vInL, vInR );
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK12();
		__vector4 vMixIn = __vaddfp( vInL, vInR );
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs3 );

		SCALEINOUTSTEREO();
		COMPUTEFEEDBACK16();
		__vector4 vMixIn = __vaddfp( vInL, vInR );
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );
	
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLS = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLS = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs3 );

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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLFE = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;
		
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLFE = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );
	
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLFE = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );
		
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
		__vector4 vOutL = __vmulfp( vOutDecorrelationVectorA, vDampedDelayOutputs0 );
		__vector4 vOutR = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutC = __vmulfp( vOutDecorrelationVectorC, vDampedDelayOutputs0 );
		__vector4 vOutLFE = __vmulfp( vOutDecorrelationVectorB, vDampedDelayOutputs0 );
		__vector4 vOutLS = __vmulfp( vOutDecorrelationVectorD, vDampedDelayOutputs0 );
		__vector4 vOutRS = __vmulfp( vOutDecorrelationVectorF, vDampedDelayOutputs0 );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs1, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs1, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs1, vOutC );
		vOutLFE = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs1, vOutLFE );
		vOutLS = __vmaddfp( vOutDecorrelationVectorE, vDampedDelayOutputs1, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorD, vMinusOne ), vDampedDelayOutputs1, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs2, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs2, vOutC );
		vOutLFE = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs2, vOutLFE );
		vOutLS = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs2, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorC, vMinusOne ), vDampedDelayOutputs2, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		vOutL = __vmaddfp( vOutDecorrelationVectorA, vDampedDelayOutputs3, vOutL );
		vOutR = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutR );
		vOutC = __vmaddfp( vOutDecorrelationVectorC, vDampedDelayOutputs3, vOutC );
		vOutLFE = __vmaddfp( __vmulfp( vOutDecorrelationVectorB, vMinusOne ), vDampedDelayOutputs3, vOutLFE );
		vOutLS = __vmaddfp( vOutDecorrelationVectorB, vDampedDelayOutputs3, vOutLS );
		vOutRS = __vmaddfp( __vmulfp( vOutDecorrelationVectorA, vMinusOne ), vDampedDelayOutputs3, vOutRS );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs3 );

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
		__vector4 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = __vmulfp( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__vector4 vScaleFactor = vDampedDelayOutputs0;
		
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
		__vector4 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = __vmulfp( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );
		
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
		__vector4 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = __vmulfp( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );
		
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
		__vector4 vOut[AK_VOICE_MAX_NUM_CHANNELS];
		do 
		{
			vOut[i] = __vmulfp( vOutDecorrelationVector[i][0], vDampedDelayOutputs0 );
			++i;
		}
		while ( i < uNumProcessedChannels );
		__vector4 vScaleFactor = vDampedDelayOutputs0;

		DAMPEDDELAYSGROUP1();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][1], vDampedDelayOutputs1, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs1 );

		DAMPEDDELAYSGROUP2();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][2], vDampedDelayOutputs2, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs2 );

		DAMPEDDELAYSGROUP3();
		i = 0;
		do 
		{
			vOut[i] = __vmaddfp( vOutDecorrelationVector[i][3], vDampedDelayOutputs3, vOut[i] );
			++i;
		}
		while ( i < uNumProcessedChannels );
		vScaleFactor = __vaddfp( vScaleFactor, vDampedDelayOutputs3 );

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
		m_vIIRLPFB0[i/4].v[i%4] = (AkReal32)( fB0*(1.0-fA1) );
		m_vIIRLPFA1[i/4].v[i%4] =  (AkReal32)fA1;
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

