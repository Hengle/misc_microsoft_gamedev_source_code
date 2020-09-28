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
// AkSimpleVerbFX.cpp
//
// SimpleVerbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkSimpleVerbFX.h"
#include <assert.h>
#include <math.h>

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
#include "AKTrace.h"
#endif

static const AkReal32 INVALIDDECAYTIMEVALUE = -1.f;
// This theshold was ear tuned to avoid interpolation artifacts for current tick rate (every 32 samples)
static const AkReal32 GAININC = 3e-3f;
// Since the increment represent constant slope the longest time to reach target is 32*(maxval-minval)/(GAININC*SR)
// The worst case is 222 ms, on average things will be much snapier.
static const AkReal32 LPFCUTFREQINC = 625.f; // range / num subblock per block

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreateReverbLiteFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkSimpleVerbFX( ) );
}


// Constructor.
CAkSimpleVerbFX::CAkSimpleVerbFX()
{
	m_fpPerformDSP = NULL;			// No func ptr set
	m_pSharedParams = NULL;
	for(int i=0; i < MAX_NUM_SUBBLOCK_BUFFERS; i++)
		m_pSubBlockBuffer[i] = NULL;

}

// Destructor.
CAkSimpleVerbFX::~CAkSimpleVerbFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkSimpleVerbFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,			// FX Context
								AK::IAkPluginParam * in_pParams,		// Effect parameters.
								AkAudioFormat &	in_rFormat			// Required audio input format.
							   )
{
	// Note: This effect is not compliant with multi-channel support on SFX
	AkChannelMask uChannelMask = in_rFormat.GetChannelMask();
	if (	uChannelMask != AK_SPEAKER_SETUP_MONO && 
			uChannelMask != AK_SPEAKER_SETUP_STEREO && 
			uChannelMask != AK_SPEAKER_SETUP_5POINT1 )
		return AK_UnsupportedChannelConfig;

	// Save format internally
	m_uSampleRate = in_rFormat.uSampleRate;

	// Set parameters.
	m_pSharedParams = reinterpret_cast<CAkSimpleVerbFXParams*>(in_pParams);
	bool bProcessLFE = m_pSharedParams->GetProcessLFE();
	AkUInt32 uNumChannels = in_rFormat.GetNumChannels( );

	// Setup DSP function ptr for current audio format
	if( in_pFXCtx->IsSendModeEffect() )
	{
		switch ( uChannelMask )
		{
		case AK_SPEAKER_SETUP_MONO:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		case AK_SPEAKER_SETUP_STEREO:
			m_fpPerformDSP = &CAkSimpleVerbFX::ProcessStereoSent;
			break;
		case AK_SPEAKER_SETUP_5POINT1:
			if ( bProcessLFE )
				m_fpPerformDSP = &CAkSimpleVerbFX::ProcessFivePointOneSent;
			else
			{
				m_fpPerformDSP = &CAkSimpleVerbFX::ProcessFivePointZeroSent;
				--uNumChannels;
			}
			break;
		default:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		}
	}
	else
	{
		switch ( uChannelMask )
		{
		case AK_SPEAKER_SETUP_MONO:
			m_fpPerformDSP = &CAkSimpleVerbFX::ProcessMono;
			break;
		case AK_SPEAKER_SETUP_STEREO:
			m_fpPerformDSP = &CAkSimpleVerbFX::ProcessStereo;
			break;
		case AK_SPEAKER_SETUP_5POINT1:
			if ( bProcessLFE )
				m_fpPerformDSP = &CAkSimpleVerbFX::ProcessFivePointOne;
			else
			{
				m_fpPerformDSP = &CAkSimpleVerbFX::ProcessFivePointZero;
				--uNumChannels;
			}
			break;
		default:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		}
	}

	// Alloc some temporary buffers for processing
	for( unsigned int i = 0; i < PluginMin( uNumChannels, MAX_NUM_SUBBLOCK_BUFFERS ); i++)
	{
		m_pSubBlockBuffer[i] = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * SUBBLOCK_FRAMESIZE );
	}

// Print out total memory allocated to Wwise debug window
#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
		AKTrace::FormatTrace( AKTrace::CategoryGeneral, L"ReverbLitet: Total allocated memory: %d\n", sizeof(AkReal32) * SUBBLOCK_FRAMESIZE  );
#endif

	// Initialize reverberator unit
	AKRESULT eResult = m_ReverbUnit.Init( in_pAllocator, m_uSampleRate, SUBBLOCK_FRAMESIZE );
	if ( eResult != AK_Success )
		return eResult;
	
	// Set initial reverb time
	m_fCachedReverbTime = m_pSharedParams->GetReverbTime();
	m_ReverbUnit.SetReverbTime( m_fCachedReverbTime );

	// Setup one pole low pass filter
	m_fCachedLPCutoffFreq = m_pSharedParams->GetCutoffFreq( );
	m_LPFilter.SetSampleRate( m_uSampleRate );
	m_LPFilter.SetCutoffFrequency( m_fCachedLPCutoffFreq );

	// Setup DC filter
	m_DCFilter.SetSampleRate( m_uSampleRate );
	m_DCFilter.SetCutoffFrequency( DCFILTERCUTOFFFREQ );

	// Tail-related initialization
	m_uSampleFramesToFlush = 0;
	m_fPrevDecayTime = INVALIDDECAYTIMEVALUE;
	m_PrevPreStop = false;

	// Gain ramp initialization for Output level, Wet/Dry and feedback levels
	m_GainRamp[OutputLevel].RampSetup( GAININC, m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].RampSetup( GAININC*100, m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[LPFCutFreq].RampSetup( LPFCUTFREQINC, m_pSharedParams->GetCutoffFreq() );

	return AK_Success;
}

// Terminates.
AKRESULT CAkSimpleVerbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	m_ReverbUnit.Term( in_pAllocator );

	// Free temporary buffers used for processing
	for(int i=0; i<MAX_NUM_SUBBLOCK_BUFFERS; i++)
	{
		if(m_pSubBlockBuffer[i] != NULL)
		{
			AK_PLUGIN_FREE( in_pAllocator, m_pSubBlockBuffer[i] );
			m_pSubBlockBuffer[i] = NULL;
		}
	}

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkSimpleVerbFX::Reset( )
{
	m_ReverbUnit.Reset(); 
	m_DCFilter.Reset();
	m_LPFilter.Reset();
	return AK_Success;
}

// Effect info query.
AKRESULT CAkSimpleVerbFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute filter effect.
//-----------------------------------------------------------------------------
void CAkSimpleVerbFX::Execute(	AkAudioBuffer* io_pBuffer )
{
	// Setup comb filter coefficients as function of reverb time
	AkReal32 fReverbTime = m_pSharedParams->GetReverbTime();
	if ( fReverbTime != m_fCachedReverbTime )
	{
		m_ReverbUnit.SetReverbTime( fReverbTime );
		m_fCachedReverbTime = fReverbTime;
	}

	// Low pass filter
	// Check if value has changed before recomputing
	AkReal32 fCutoffFreq = m_pSharedParams->GetCutoffFreq( );
	if ( fCutoffFreq != m_fCachedLPCutoffFreq )
	{
		m_LPFilter.SetCutoffFrequency( fCutoffFreq );
		m_fCachedLPCutoffFreq = fCutoffFreq;
	}

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

	// Dereference required perform method
	(this->*m_fpPerformDSP)( io_pBuffer  );

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
#endif
}

void CAkSimpleVerbFX::ProcessMono( AkAudioBuffer * io_pBuffer )
{
	AkReal32 * AK_RESTRICT pBuffer = io_pBuffer->GetChannel(0);
	AkReal32 * AK_RESTRICT pReverbBuffer = m_pSubBlockBuffer[0];

	// Prefetch filter memory
	m_ReverbUnit.PrimeCombFiltersCurrent( );
	m_ReverbUnit.PrimeAllPassFiltersCurrent( );

	// Change target gain if necessary (RTPC value)
	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	// Determine how many samples will be processed and handle FX tail
	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetReverbTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fDecayTime * m_uSampleRate );
			m_fPrevDecayTime = fDecayTime;
		}	
		AkUInt32 uSampleFramesToLoop = PluginMin( m_uSampleFramesToFlush, (io_pBuffer->MaxFrames()-ulNumSampleFrames) );
		AKPLATFORM::AkMemSet( (pBuffer + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		ulNumSampleFrames += uSampleFramesToLoop;
		m_uSampleFramesToFlush -= uSampleFramesToLoop;
		if ( m_uSampleFramesToFlush > 0 )
			io_pBuffer->eState = AK_DataReady;
	}

	// DC filter on whole buffer (does not require temporary buffer)
	m_DCFilter.ProcessBuffer( pBuffer, ulNumSampleFrames );

	// Sub block loop
	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;
		
		// Network of comb and allpass filters
		m_ReverbUnit.ProcessBuffer( pBuffer, pReverbBuffer, ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		if ( fLPFCutFreq != m_fCachedLPCutoffFreq )
		{
			m_LPFilter.SetCutoffFrequency( fLPFCutFreq );
			m_fCachedLPCutoffFreq = fLPFCutFreq;
		}
		m_LPFilter.ProcessBuffer( pReverbBuffer, ulFramesToProcessThisLoop );

		// Compute interpolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fCurWetDry = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fDryLevel = ( SIMPLEVERBFXPARAM_WETDRYMIX_MAX - fCurWetDry ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{		
			AkReal32Vector vWet, vDry, vRev;
			AKSIMD_LOADVEC( pReverbBuf, vWet );
			vWet = AKSIMD_MUL( vWet, vWetLevel );
			pReverbBuf+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pBuffer, vWet, vDry, vRev );
			pBuffer+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fScaledWet = fWetLevel * *pReverbBuf;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pBuffer, fScaledWet );
			pBuffer++;
			pReverbBuf++;
		}
	}

	// Make denormals zero in feedback memories to stop propagation
	m_LPFilter.AvoidDenormal();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

void CAkSimpleVerbFX::ProcessStereo( AkAudioBuffer * io_pBuffer )
{
	AkReal32 * AK_RESTRICT pChannelL = io_pBuffer->GetChannel(AK_IDX_SETUP_2_LEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = io_pBuffer->GetChannel(AK_IDX_SETUP_2_RIGHT);		// Right
	AkReal32 * AK_RESTRICT pMixedBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[0];
	AkReal32 * AK_RESTRICT pReverbBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[1];

	// Prefetch filter memory
	m_ReverbUnit.PrimeCombFiltersCurrent( );
	m_ReverbUnit.PrimeAllPassFiltersCurrent( );

	// Change target gain if necessary (RTPC value)
	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	// Determine how many samples will be processed and handle FX tail
	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetReverbTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fDecayTime * m_uSampleRate );
			m_fPrevDecayTime = fDecayTime;
		}	
		AkUInt32 uSampleFramesToLoop = PluginMin( m_uSampleFramesToFlush, (io_pBuffer->MaxFrames()-ulNumSampleFrames) );
		AKPLATFORM::AkMemSet( (pChannelL + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelR + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		ulNumSampleFrames += uSampleFramesToLoop;
		m_uSampleFramesToFlush -= uSampleFramesToLoop;
		if ( m_uSampleFramesToFlush > 0 )
			io_pBuffer->eState = AK_DataReady;
	}

	// Sub block loop
	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		// fReverbIn = ( fDryL + fDryR ) * STEREONORMFACTOR;
		AddAndMultiply( pChannelL, pChannelR, STEREONORMFACTOR, pMixedBuffer, ulFramesToProcessThisLoop );

		// DC filter
		m_DCFilter.ProcessBuffer( pMixedBuffer, ulFramesToProcessThisLoop );
		
		// Network of comb and allpass filters
		m_ReverbUnit.ProcessBuffer( pMixedBuffer, pReverbBuffer, ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		if ( fLPFCutFreq != m_fCachedLPCutoffFreq )
		{
			m_LPFilter.SetCutoffFrequency( fLPFCutFreq );
			m_fCachedLPCutoffFreq = fLPFCutFreq;
		}
		m_LPFilter.ProcessBuffer( pReverbBuffer, ulFramesToProcessThisLoop );

		// Compute intepolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fIntWetDry = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fDryLevel = ( SIMPLEVERBFXPARAM_WETDRYMIX_MAX - fIntWetDry ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vWet, vDry, vRev;
			AKSIMD_LOADVEC( pReverbBuf, vWet );
			vWet = AKSIMD_MUL( vWet, vWetLevel );

			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelL, vWet, vDry, vRev );
			pChannelL+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelR, vWet, vDry, vRev );
			pChannelR+=4;

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fScaledWet = fWetLevel * *pReverbBuf;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelL, fScaledWet );
			++pChannelL;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelR, fScaledWet );
			++pChannelR;
			++pReverbBuf;
		}
	}

	// Make denormals zero in feedback memories to stop propagation
	m_LPFilter.AvoidDenormal();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

// Process five point one signals as using wet/dry levels (insert mode) with LFE passthrough
void CAkSimpleVerbFX::ProcessFivePointZero( AkAudioBuffer * io_pBuffer )
{
	// Setup pointers to each channel buffers
	AkReal32 * AK_RESTRICT pChannelL = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTLEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTRIGHT);		// Right
	AkReal32 * AK_RESTRICT pChannelC = io_pBuffer->GetChannel(AK_IDX_SETUP_5_CENTER);			// Center
	AkReal32 * AK_RESTRICT pChannelLS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARLEFT);		// Left surround
	AkReal32 * AK_RESTRICT pChannelRS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARRIGHT);		// Right surround
	AkReal32 * AK_RESTRICT pChannelLFE = io_pBuffer->GetChannel(AK_IDX_SETUP_5_LFE);			// LFE
	AkReal32 * AK_RESTRICT pMixedBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[0];
	AkReal32 * AK_RESTRICT pReverbBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[1];

	// Prefetch filter memory
	m_ReverbUnit.PrimeCombFiltersCurrent( );
	m_ReverbUnit.PrimeAllPassFiltersCurrent( );

	// Change target gain if necessary (RTPC value)
	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	// Determine how many samples will be processed and handle FX tail
	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetReverbTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fDecayTime * m_uSampleRate );
			m_fPrevDecayTime = fDecayTime;
		}	
		AkUInt32 uSampleFramesToLoop = PluginMin( m_uSampleFramesToFlush, (io_pBuffer->MaxFrames()-ulNumSampleFrames) );
		AKPLATFORM::AkMemSet( (pChannelL + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelR + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelC + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelLS + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelRS + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelLFE + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		ulNumSampleFrames += uSampleFramesToLoop;
		m_uSampleFramesToFlush -= uSampleFramesToLoop;
		if ( m_uSampleFramesToFlush > 0 )
			io_pBuffer->eState = AK_DataReady;
	}

	// Sub block loop
	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		// fReverbIn = ( fDryL + fDryR + fDryC + fDryLS + fDryRS ) * FIVEPOINTZERONORMFACTOR;
		AddAndMultiply( pChannelL, pChannelR, pChannelC, pChannelLS, pChannelRS, 
						FIVEPOINTZERONORMFACTOR, pMixedBuffer, ulFramesToProcessThisLoop );

		// DC filter
		m_DCFilter.ProcessBuffer( pMixedBuffer, ulFramesToProcessThisLoop );
		
		// Network of comb and allpass filters
		m_ReverbUnit.ProcessBuffer( pMixedBuffer, pReverbBuffer, ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		if ( fLPFCutFreq != m_fCachedLPCutoffFreq )
		{
			m_LPFilter.SetCutoffFrequency( fLPFCutFreq );
			m_fCachedLPCutoffFreq = fLPFCutFreq;
		}
		m_LPFilter.ProcessBuffer( pReverbBuffer, ulFramesToProcessThisLoop );

		// Compute intepolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fIntWetDry = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fDryLevel = ( SIMPLEVERBFXPARAM_WETDRYMIX_MAX - fIntWetDry ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vWet, vDry, vRev;
			AKSIMD_LOADVEC( pReverbBuf, vWet );
			vWet = AKSIMD_MUL( vWet, vWetLevel );

			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelL, vWet, vDry, vRev );
			pChannelL+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelR, vWet, vDry, vRev );
			pChannelR+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelC, vWet, vDry, vRev );
			pChannelC+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelLS, vWet, vDry, vRev );
			pChannelLS+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelRS, vWet, vDry, vRev );
			pChannelRS+=4;

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fScaledWet = fWetLevel * *pReverbBuf;

			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelL, fScaledWet );
			++pChannelL;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelR, fScaledWet );
			++pChannelR;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelC, fScaledWet );
			++pChannelC;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelLS, fScaledWet );
			++pChannelLS;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelRS, fScaledWet );
			++pChannelRS;

			++pReverbBuf;
		}
	}

	// Make denormals zero in feedback memories to stop propagation
	m_LPFilter.AvoidDenormal();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}
//====================================================================================================

// Process five point one signals as using wet/dry levels (insert mode) with reverb on LFE as well
void CAkSimpleVerbFX::ProcessFivePointOne( AkAudioBuffer * io_pBuffer )
{
	// Setup pointers to each channel buffers
	AkReal32 * AK_RESTRICT pChannelL = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTLEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTRIGHT);		// Right
	AkReal32 * AK_RESTRICT pChannelC = io_pBuffer->GetChannel(AK_IDX_SETUP_5_CENTER);			// Center
	AkReal32 * AK_RESTRICT pChannelLS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARLEFT);		// Left surround
	AkReal32 * AK_RESTRICT pChannelRS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARRIGHT);		// Right surround
	AkReal32 * AK_RESTRICT pChannelLFE = io_pBuffer->GetChannel(AK_IDX_SETUP_5_LFE);			// LFE
	AkReal32 * AK_RESTRICT pMixedBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[0];
	AkReal32 * AK_RESTRICT pReverbBuffer = (AkReal32 * AK_RESTRICT) m_pSubBlockBuffer[1];

	// Prefetch filter memory
	m_ReverbUnit.PrimeCombFiltersCurrent( );
	m_ReverbUnit.PrimeAllPassFiltersCurrent( );

	// Change target gain if necessary (RTPC value)
	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	// Determine how many samples will be processed and handle FX tail
	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetReverbTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fDecayTime * m_uSampleRate );
			m_fPrevDecayTime = fDecayTime;
		}	
		AkUInt32 uSampleFramesToLoop = PluginMin( m_uSampleFramesToFlush, (io_pBuffer->MaxFrames()-ulNumSampleFrames) );
		AKPLATFORM::AkMemSet( (pChannelL + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelR + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelC + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelLS + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelRS + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( (pChannelLFE + ulNumSampleFrames), 0, uSampleFramesToLoop * sizeof(AkReal32) );
		ulNumSampleFrames += uSampleFramesToLoop;
		m_uSampleFramesToFlush -= uSampleFramesToLoop;
		if ( m_uSampleFramesToFlush > 0 )
			io_pBuffer->eState = AK_DataReady;
	}

	// Sub block loop
	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		// fReverbIn = ( fDryL + fDryR + fDryC + fDryLS + fDryRS + fDryLFE ) * FIVEPOINTONENORMFACTOR;
		AddAndMultiply( pChannelL, pChannelR, pChannelC, pChannelLS, pChannelRS, pChannelLFE, 
						FIVEPOINTONENORMFACTOR, pMixedBuffer, ulFramesToProcessThisLoop );

		// DC filter
		m_DCFilter.ProcessBuffer( pMixedBuffer, ulFramesToProcessThisLoop );
		
		// Network of comb and allpass filters
		m_ReverbUnit.ProcessBuffer( pMixedBuffer, pReverbBuffer, ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		if ( fLPFCutFreq != m_fCachedLPCutoffFreq )
		{
			m_LPFilter.SetCutoffFrequency( fLPFCutFreq );
			m_fCachedLPCutoffFreq = fLPFCutFreq;
		}
		m_LPFilter.ProcessBuffer( pReverbBuffer, ulFramesToProcessThisLoop );

		// Compute intepolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fIntWetDry = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fDryLevel = ( SIMPLEVERBFXPARAM_WETDRYMIX_MAX - fIntWetDry ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vWet, vDry, vRev;
			AKSIMD_LOADVEC( pReverbBuf, vWet );
			vWet = AKSIMD_MUL( vWet, vWetLevel );

			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelL, vWet, vDry, vRev );
			pChannelL+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelR, vWet, vDry, vRev );
			pChannelR+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelC, vWet, vDry, vRev );
			pChannelC+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelLS, vWet, vDry, vRev );
			pChannelLS+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelRS, vWet, vDry, vRev );
			pChannelRS+=4;
			SCALEREVERBCOMPONENTSSIMD( vGain, vDryLevel, pChannelLFE, vWet, vDry, vRev );
			pChannelLFE+=4;

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fScaledWet = fWetLevel * *pReverbBuf;

			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelL, fScaledWet );
			++pChannelL;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelR, fScaledWet );
			++pChannelR;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelC, fScaledWet );
			++pChannelC;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelLS, fScaledWet );
			++pChannelLS;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelRS, fScaledWet );
			++pChannelRS;
			SCALEREVERBCOMPONENTS( fCurOutGain, fDryLevel, pChannelLFE, fScaledWet );
			++pChannelLFE;

			++pReverbBuf;
		}
	}

	// Make denormals zero in feedback memories to stop propagation
	m_LPFilter.AvoidDenormal();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}
//====================================================================================================