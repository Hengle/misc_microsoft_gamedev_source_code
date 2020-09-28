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
// AkReverbFX.cpp
//
// ReverbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkReverbFX.h"
#include <math.h>
#include <assert.h>
#include <new>
#undef new

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
AK::IAkPlugin* CreateReverbFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkReverbFX( ) );
}

// Constructor.
CAkReverbFX::CAkReverbFX()
{
	m_pSharedParams = NULL;
	m_pLPFilter = NULL;
	// Signal processing elements
	m_pReflectionsDelay = NULL;		// 1 per channel
	m_pReverbDelay = NULL;			// 1 per channel
	m_pMultiTapDelay = NULL;		// 1 per channel
	m_pReverbUnit = NULL;
	m_pDualReverbUnit = NULL;

	for(int i=0; i < MAX_NUM_SUBBLOCK_BUFFERS; i++)
		m_pSubBlockBuffer[i] = NULL;

	// Processing function
	m_fpPerformDSP = NULL;			// No func ptr set
}

// Destructor.
CAkReverbFX::~CAkReverbFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkReverbFX::Init(		AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
								AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context	
								AK::IAkPluginParam * in_pParams,			// Effect parameters.
								AkAudioFormat &		in_rFormat				// Required audio input format.
						   )
{
	// Note: This effect is not compliant with multi-channel support on SFX
	AkChannelMask uChannelMask = in_rFormat.GetChannelMask();
	if (	uChannelMask != AK_SPEAKER_SETUP_MONO && 
			uChannelMask != AK_SPEAKER_SETUP_STEREO && 
			uChannelMask != AK_SPEAKER_SETUP_5POINT1 )
		return AK_UnsupportedChannelConfig;

	AKRESULT eResult = AK_Fail;
	// Set parameters.
	m_pSharedParams = reinterpret_cast<CAkReverbFXParams*>(in_pParams);

	// Save format internally
	m_uNumChannels = static_cast<AkUInt8>( in_rFormat.GetNumChannels( ) );
	m_uSampleRate = in_rFormat.uSampleRate;
	bool bIsSentFX = in_pFXCtx->IsSendModeEffect();

	// Determine how many reverberator units to use (can we implement stereo width)
	AkUInt32 uNumReverbUnits;
	if ( m_uNumChannels < 2 )
	{
		uNumReverbUnits = 1;
	}
	else
	{
		uNumReverbUnits = 2;
	}

	// Alloc m_uNumChannels temporary buffers for processing
	AkUInt32 uNumTempBuffers;
	bool bProcessLFE = m_pSharedParams->GetProcessLFE();

	// Setup DSP function ptr for current audio format
	if( bIsSentFX )
	{
		switch ( uChannelMask )
		{
		case AK_SPEAKER_SETUP_MONO:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		case AK_SPEAKER_SETUP_STEREO:
			m_fpPerformDSP = &CAkReverbFX::ProcessStereoSent;
			break;
		case AK_SPEAKER_SETUP_5POINT1:
			if ( bProcessLFE )
				m_fpPerformDSP = &CAkReverbFX::ProcessFivePointOneSent;
			else
			{
				m_fpPerformDSP = &CAkReverbFX::ProcessFivePointZeroSent;
				--m_uNumChannels;
			}
			break;
		default:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		}

		uNumTempBuffers = (m_uNumChannels + uNumReverbUnits + 1);
	}
	else
	{
		switch ( uChannelMask )
		{
		case AK_SPEAKER_SETUP_MONO:
			m_fpPerformDSP = &CAkReverbFX::ProcessMono;
			break;
		case AK_SPEAKER_SETUP_STEREO:
			m_fpPerformDSP = &CAkReverbFX::ProcessStereo;
			break;
		case AK_SPEAKER_SETUP_5POINT1:
			if ( bProcessLFE )
				m_fpPerformDSP = &CAkReverbFX::ProcessFivePointOne;
			else
			{
				m_fpPerformDSP = &CAkReverbFX::ProcessFivePointZero;
				--m_uNumChannels;
			}
			break;
		default:
			m_fpPerformDSP = NULL;
			return AK_UnsupportedChannelConfig;
		}

		uNumTempBuffers = PluginMax((2*m_uNumChannels), m_uNumChannels+uNumReverbUnits+1);
	}

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
	AkUInt32 uTotalMemoryAllocated = 0;
#endif

	for( unsigned int i = 0; i < uNumTempBuffers; i++)
	{
		m_pSubBlockBuffer[i] = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * SUBBLOCK_FRAMESIZE );
		if ( m_pSubBlockBuffer[i] == NULL )
			return AK_InsufficientMemory;

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(AkReal32) * SUBBLOCK_FRAMESIZE;
#endif
	}
    
	// Reverberator units
	m_fCachedReverbTime = m_pSharedParams->GetDecayTime();
	if ( uNumReverbUnits == 1 )
	{
		m_pReverbUnit = AK_PLUGIN_NEW( in_pAllocator, CAkReverbUnit );
		if ( !m_pReverbUnit )
			return AK_InsufficientMemory;
		// Initialize reverberator unit
		eResult = m_pReverbUnit->Init( in_pAllocator, m_uSampleRate, SUBBLOCK_FRAMESIZE );
		if ( eResult != AK_Success )
			return eResult;
		m_pReverbUnit->SetReverbTime( m_fCachedReverbTime );
	}
	else
	{
		m_pDualReverbUnit = AK_PLUGIN_NEW( in_pAllocator, CAkDualReverbUnit );
		if ( !m_pDualReverbUnit )
			return AK_InsufficientMemory;
		// Initialize reverberator unit
		eResult = m_pDualReverbUnit->Init( in_pAllocator, m_uSampleRate, SUBBLOCK_FRAMESIZE );
		if ( eResult != AK_Success )
			return eResult;
		// Set initial reverb time
		m_pDualReverbUnit->SetReverbTime( m_fCachedReverbTime );
	}

	// Alloc low pass filters
	m_uNumLPFilters = 2 * uNumReverbUnits;
	m_pLPFilter = (CAkOnePoleLPFilter*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(CAkOnePoleLPFilter) * m_uNumLPFilters );
	if ( !m_pLPFilter )
		return AK_InsufficientMemory;

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(CAkOnePoleLPFilter) * m_uNumLPFilters;
#endif

	// Setup low pass filters
	m_fCachedLPCutoffFreq = m_pSharedParams->GetCutoffFreq( );
	for ( unsigned int i = 0; i < m_uNumLPFilters; ++i )
	{
		::new( &(m_pLPFilter[i]) ) CAkOnePoleLPFilter; // placement new
		m_pLPFilter[i].SetSampleRate( m_uSampleRate );
		m_pLPFilter[i].SetCutoffFrequency( m_fCachedLPCutoffFreq );
	}

	// Setup DC filter
	m_DCFilter.SetSampleRate( m_uSampleRate );
	m_DCFilter.SetCutoffFrequency( DCFILTERCUTOFFFREQ );

	// Early reflection units (1 per input channel)
	m_pMultiTapDelay = (CAkMultiTapDelay*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(CAkMultiTapDelay) * m_uNumChannels );
	if ( !m_pMultiTapDelay )
		return AK_InsufficientMemory;

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
	uTotalMemoryAllocated += sizeof(CAkMultiTapDelay) * m_uNumChannels;
#endif
 
	for ( unsigned int i = 0; i < m_uNumChannels; ++i )
		::new( &(m_pMultiTapDelay[i]) ) CAkMultiTapDelay; // placement new

	// Init multi-tap delays
	for ( unsigned int i = 0; i < m_uNumChannels; ++i )
	{
		eResult = m_pMultiTapDelay[i].Init( in_pAllocator, m_uSampleRate );
		if ( eResult != AK_Success )
		{
			return eResult;
		}
	}

	// Reflections delay
	m_pReflectionsDelay = (CAkDelay*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(CAkDelay) * m_uNumChannels );
	if ( !m_pReflectionsDelay )
		return AK_InsufficientMemory;

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
	uTotalMemoryAllocated += sizeof(CAkDelay) * m_uNumChannels;
#endif

	// Setup reflection delay
	AkUInt32 ulReflectionsDelay = static_cast<AkUInt32>( m_pSharedParams->GetReflectionsDelay( ) * m_uSampleRate );

	for ( unsigned int i = 0; i < m_uNumChannels; i++ )
		::new( &(m_pReflectionsDelay[i]) ) CAkDelay; // placement new

	for ( unsigned int i = 0; i < m_uNumChannels; i++ )
	{
		eResult = m_pReflectionsDelay[i].Init( in_pAllocator, ulReflectionsDelay );
		if ( eResult != AK_Success )
		{
			return eResult;
		}
	}

	// Reverb delay
	m_pReverbDelay = (CAkDelay*) AK_PLUGIN_ALLOC( in_pAllocator, sizeof(CAkDelay) * m_uNumChannels );
	if ( !m_pReverbDelay )
		return AK_InsufficientMemory;

#if defined(_DEBUG) && defined(WIN32) && defined(MEMORYOUTPUT)
		uTotalMemoryAllocated += sizeof(CAkDelay) * m_uNumChannels;
#endif

	// Setup reverb delay line
	AkUInt32 ulReverbDelay = static_cast<AkUInt32>( m_pSharedParams->GetReverbDelay( ) * m_uSampleRate );

	for ( unsigned int i = 0; i < m_uNumChannels; i++ )
		::new( &(m_pReverbDelay[i]) ) CAkDelay; // placement new

	for ( unsigned int i = 0; i < m_uNumChannels; i++ )
	{
		eResult = m_pReverbDelay[i].Init( in_pAllocator, ulReverbDelay );
		if ( eResult != AK_Success )
		{
			return eResult;
		}
	}

	// Tail-related initialization
	m_uSampleFramesToFlush = 0;
	m_fPrevDecayTime = INVALIDDECAYTIMEVALUE;
	m_PrevPreStop = false;

	// Gain ramps initialization
	m_GainRamp[OutputLevel].RampSetup( GAININC, m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].RampSetup( GAININC*100, m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[ReverbLevel].RampSetup( GAININC, m_pSharedParams->GetReverbLevel( ) );
	m_GainRamp[EarlyRefLevel].RampSetup( GAININC, m_pSharedParams->GetReflectionsLevel( ) );
	m_GainRamp[LPFCutFreq].RampSetup( LPFCUTFREQINC, m_pSharedParams->GetCutoffFreq() );
	if ( uNumReverbUnits == 2 )
		m_GainRamp[ReverbWidth].RampSetup( GAININC, m_pSharedParams->GetReverbWidth( ) );

	return AK_Success;
}

// Terminates.
AKRESULT CAkReverbFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );

	if ( m_pReverbUnit )
	{
		m_pReverbUnit->Term( in_pAllocator );
		AK_PLUGIN_DELETE( in_pAllocator, m_pReverbUnit );
		m_pReverbUnit = NULL;
	}
	else if ( m_pDualReverbUnit )
	{
		m_pDualReverbUnit->Term( in_pAllocator );
		AK_PLUGIN_DELETE( in_pAllocator, m_pDualReverbUnit );
		m_pDualReverbUnit = NULL;
	}

	if ( m_pReverbDelay != NULL )
	{
		for ( unsigned int i = 0; i < m_uNumChannels; i++ )
		{
			m_pReverbDelay[i].Term( in_pAllocator );
		}	
		AK_PLUGIN_FREE( in_pAllocator, m_pReverbDelay );
		m_pReverbDelay = NULL;
	}

	if ( m_pReflectionsDelay != NULL )
	{	
		for ( unsigned int i = 0; i < m_uNumChannels; i++ )
		{
			m_pReflectionsDelay[i].Term( in_pAllocator );
		}	
		AK_PLUGIN_FREE( in_pAllocator, m_pReflectionsDelay );
		m_pReflectionsDelay = NULL;
	}

	// Early reflection units ( 1 per channel)
	if ( m_pMultiTapDelay != NULL )
	{
		for ( unsigned int i = 0; i < m_uNumChannels; i++ )
		{
			m_pMultiTapDelay[i].Term( in_pAllocator );
		}
		AK_PLUGIN_FREE( in_pAllocator, m_pMultiTapDelay );
		m_pMultiTapDelay = NULL;
	}

	// Low pass filters
	if ( m_pLPFilter != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pLPFilter );
		m_pLPFilter = NULL;
	}
		
	// Free temporary buffers used for processing
	for(int i=0; i < MAX_NUM_SUBBLOCK_BUFFERS; i++)
	{
		if(m_pSubBlockBuffer[i] != NULL)
		{
			AK_PLUGIN_FREE( in_pAllocator, m_pSubBlockBuffer[i] );
			m_pSubBlockBuffer[i] = NULL;
		}
	}

	// Effect's deletion
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Reset or seek to start.
AKRESULT CAkReverbFX::Reset( )
{
	// Reset all filters
	if ( m_pReverbUnit )
	{
		m_pReverbUnit->Reset( );
	}

	else if ( m_pDualReverbUnit )
	{
		m_pDualReverbUnit->Reset( );
	}

	// Early reflection units ( 1 per channel)
	for ( unsigned int i = 0; i < m_uNumChannels; i++ )
	{
		if ( m_pMultiTapDelay != NULL )
			m_pMultiTapDelay[i].Reset( );
		if ( m_pReflectionsDelay != NULL )
			m_pReflectionsDelay[i].Reset( );
		if ( m_pReverbDelay != NULL )
			m_pReverbDelay[i].Reset( );
	}

	m_DCFilter.Reset();
	ResetLPFilters();

	return AK_Success;
}

// Effect info query.
AKRESULT CAkReverbFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
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

void CAkReverbFX::Execute(	AkAudioBuffer * io_pBuffer )
{
	// Setup comb filter coefficients as function of reverb time
	AkReal32 fReverbTime = m_pSharedParams->GetDecayTime();
	if ( fReverbTime != m_fCachedReverbTime )
	{
		if ( m_pReverbUnit )
			m_pReverbUnit->SetReverbTime( fReverbTime );
		else 
		{
			assert( m_pDualReverbUnit );
			m_pDualReverbUnit->SetReverbTime( fReverbTime );
		}
		m_fCachedReverbTime = fReverbTime;
	}

	// Low pass filter
	AkReal32 fCutoffFreq = m_pSharedParams->GetCutoffFreq( );
	SetLPFCutoffFrequency( fCutoffFreq );

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif

	// Dereference required perform method
	(this->*m_fpPerformDSP)( io_pBuffer );

#ifdef WIN32
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
#endif
}

void CAkReverbFX::ProcessMono( AkAudioBuffer * io_pBuffer )
{
	assert( m_pReverbUnit );

	// Prefetch filter memory
	m_pReverbUnit->PrimeCombFiltersCurrent( );
	m_pReverbUnit->PrimeAllPassFiltersCurrent( );

	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[ReverbLevel].SetTarget( m_pSharedParams->GetReverbLevel( ) );
	m_GainRamp[EarlyRefLevel].SetTarget( m_pSharedParams->GetReflectionsLevel( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	AkReal32 * AK_RESTRICT pBuffer = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(0);

	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetDecayTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			AkReal32 fTotalReverbTime = fDecayTime + m_pSharedParams->GetReverbDelay() + m_pSharedParams->GetReflectionsDelay();
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fTotalReverbTime * m_uSampleRate );
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

	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		m_pReflectionsDelay[0].ProcessBuffer( pBuffer, m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		// Network of comb and allpass filters
		m_pReverbUnit->ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq );
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );

		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetDryMix = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fDryLevel = ( REVERB_WETDRYMIX_MAX - fWetDryMix ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTap = m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pReverbBuf = m_pSubBlockBuffer[2];
		
		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain, vRefLevel, vReverbLevel;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		while ( uBlock4-- )
		{		
			SCALEREVERBCOMPONENTSSINGLESIMD( vGain, vDryLevel, pBuffer, vWetLevel, vRefLevel, pMultiTap, vReverbLevel, pReverbBuf );
			pBuffer+=4;
			pMultiTap+=4;
			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			SCALEREVERBCOMPONENTSSINGLE( fOutGain, fDryLevel, pBuffer, fWetLevel, fReflectionsLevel, pMultiTap, fReverbLevel, pReverbBuf );
			++pBuffer;
			++pMultiTap;
			++pReverbBuf;
		}
	}
	// Make denormals zero in feedback memories to stop propagation
	LPFAvoidDenormals();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

void CAkReverbFX::ProcessStereo( AkAudioBuffer * io_pBuffer )
{
	assert( m_pDualReverbUnit );

	// Prefetch filter memory
	m_pDualReverbUnit->PrimeCombFiltersCurrent( );
	m_pDualReverbUnit->PrimeAllPassFiltersCurrent( );

	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[ReverbLevel].SetTarget( m_pSharedParams->GetReverbLevel( ) );
	m_GainRamp[EarlyRefLevel].SetTarget( m_pSharedParams->GetReflectionsLevel( ) );
	m_GainRamp[ReverbWidth].SetTarget( m_pSharedParams->GetReverbWidth( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	AkReal32 * AK_RESTRICT pChannelL = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(AK_IDX_SETUP_2_LEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = (AkReal32 * AK_RESTRICT)io_pBuffer->GetChannel(AK_IDX_SETUP_2_RIGHT);		// Right

	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetDecayTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			AkReal32 fTotalReverbTime = fDecayTime + m_pSharedParams->GetReverbDelay() + m_pSharedParams->GetReflectionsDelay();
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fTotalReverbTime * m_uSampleRate );
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

	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		m_pReflectionsDelay[0].ProcessBuffer( pChannelL, m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );

		// fReverbIn = ( fReverbDelayL + fReverbDelayR ) * STEREONORMFACTOR;
		AddAndMultiply( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], STEREONORMFACTOR, m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		
		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		
		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], m_pSubBlockBuffer[4], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq );
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		
		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetDryMix = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fDryLevel = ( REVERB_WETDRYMIX_MAX - fWetDryMix ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[4];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		AKSIMD_LOAD1( fRevUnitScale1, vUnitScale1Level1 );
		AKSIMD_LOAD1( fRevUnitScale2, vUnitScale1Level2 );
		while ( uBlock4-- )
		{	
			// Cross talk between reverberator units to create stereo spread
			AkReal32Vector vReverbMix1, vReverbMix2;
			COMPUTEREVERBUNITMIXSIMD( vReverbLevel, pReverbBuf1, pReverbBuf2, vUnitScale1Level1, vUnitScale1Level2, vReverbMix1, vReverbMix2 );
			pReverbBuf1+=4;
			pReverbBuf2+=4;

			AkReal32Vector __vDry__, __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelL, vWetLevel, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelR, vWetLevel, vRefLevel, pMultiTapR, vReverbMix2 );
			pChannelR+=4;
			pMultiTapR+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fReverbMix1,fReverbMix2, __Unit1__, __Unit2__;
			COMPUTEREVERBUNITMIX( fReverbLevel, pReverbBuf1, pReverbBuf2, fRevUnitScale1, fRevUnitScale2, &fReverbMix1, &fReverbMix2 );
			++pReverbBuf1;
			++pReverbBuf2;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelL, fWetLevel, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelR, fWetLevel, fReflectionsLevel, pMultiTapR, fReverbMix2 );
			++pChannelR;
			++pMultiTapR;
		}
	}
	// Make denormals zero in feedback memories to stop propagation
	LPFAvoidDenormals( );
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

void CAkReverbFX::ProcessFivePointZero( AkAudioBuffer * io_pBuffer )
{
	assert( m_pDualReverbUnit );

	// Prefetch filter memory
	m_pDualReverbUnit->PrimeCombFiltersCurrent( );
	m_pDualReverbUnit->PrimeAllPassFiltersCurrent( );

	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[ReverbLevel].SetTarget( m_pSharedParams->GetReverbLevel( ) );
	m_GainRamp[EarlyRefLevel].SetTarget( m_pSharedParams->GetReflectionsLevel( ) );
	m_GainRamp[ReverbWidth].SetTarget( m_pSharedParams->GetReverbWidth( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	AkReal32 * AK_RESTRICT pChannelL = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTLEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTRIGHT);		// Right
	AkReal32 * AK_RESTRICT pChannelC = io_pBuffer->GetChannel(AK_IDX_SETUP_5_CENTER);			// Center
	AkReal32 * AK_RESTRICT pChannelLS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARLEFT);		// Left surround
	AkReal32 * AK_RESTRICT pChannelRS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARRIGHT);		// Right surround
	AkReal32 * AK_RESTRICT pChannelLFE = io_pBuffer->GetChannel(AK_IDX_SETUP_5_LFE);			// LFE

	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetDecayTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			AkReal32 fTotalReverbTime = fDecayTime + m_pSharedParams->GetReverbDelay() + m_pSharedParams->GetReflectionsDelay();
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fTotalReverbTime * m_uSampleRate );
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

	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		m_pReflectionsDelay[0].ProcessBuffer( pChannelL, m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[2].ProcessBuffer( pChannelC, m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[2].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pReverbDelay[2].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[3].ProcessBuffer( pChannelLS, m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[3].ProcessBuffer( m_pSubBlockBuffer[6], m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pReverbDelay[3].ProcessBuffer( m_pSubBlockBuffer[6], m_pSubBlockBuffer[7], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[4].ProcessBuffer( pChannelRS, m_pSubBlockBuffer[8], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[4].ProcessBuffer( m_pSubBlockBuffer[8], m_pSubBlockBuffer[8], ulFramesToProcessThisLoop );
		m_pReverbDelay[4].ProcessBuffer( m_pSubBlockBuffer[8], m_pSubBlockBuffer[9], ulFramesToProcessThisLoop );

		AddAndMultiply( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], m_pSubBlockBuffer[5],
						m_pSubBlockBuffer[7], m_pSubBlockBuffer[9], FIVEPOINTZERONORMFACTOR,
						m_pSubBlockBuffer[1], ulFramesToProcessThisLoop);

		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], m_pSubBlockBuffer[5], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq ); 
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		
		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetDryMix = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fDryLevel = ( REVERB_WETDRYMIX_MAX - fWetDryMix ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pMultiTapC = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[4];
		AkReal32 * AK_RESTRICT pMultiTapLS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[6];
		AkReal32 * AK_RESTRICT pMultiTapRS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[8];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[5];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		AKSIMD_LOAD1( fRevUnitScale1, vUnitScale1Level1 );
		AKSIMD_LOAD1( fRevUnitScale2, vUnitScale1Level2 );
		while ( uBlock4-- )
		{	
			// Cross talk between reverberator units to create stereo spread
			AkReal32Vector vReverbMix1, vReverbMix2, vReverbMix3;
			COMPUTEREVERBUNITMIXSIMDSURROUND( vReverbLevel, pReverbBuf1, pReverbBuf2, vUnitScale1Level1, vUnitScale1Level2, vReverbMix1, vReverbMix2, vReverbMix3 );
			pReverbBuf1+=4;
			pReverbBuf2+=4;

			AkReal32Vector __vDry__, __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelL, vWetLevel, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelR, vWetLevel, vRefLevel, pMultiTapR, vReverbMix2 );
			pChannelR+=4;
			pMultiTapR+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelC, vWetLevel, vRefLevel, pMultiTapC, vReverbMix3 );
			pChannelC+=4;
			pMultiTapC+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelLS, vWetLevel, vRefLevel, pMultiTapLS, vReverbMix1 );
			pChannelLS+=4;
			pMultiTapLS+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelRS, vWetLevel, vRefLevel, pMultiTapRS, vReverbMix2 );
			pChannelRS+=4;
			pMultiTapRS+=4;

			// LFE passthrough
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fReverbMix1,fReverbMix2,fReverbMix3,__Unit1__, __Unit2__;
			COMPUTEREVERBUNITMIXSURROUND( fReverbLevel, pReverbBuf1, pReverbBuf2, fRevUnitScale1, fRevUnitScale2, &fReverbMix1, &fReverbMix2, &fReverbMix3 );
			++pReverbBuf1;
			++pReverbBuf2;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelL, fWetLevel, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelR, fWetLevel, fReflectionsLevel, pMultiTapR, fReverbMix2 );
			++pChannelR;
			++pMultiTapR;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelC, fWetLevel, fReflectionsLevel, pMultiTapC, fReverbMix3 );
			++pChannelC;
			++pMultiTapC;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelLS, fWetLevel, fReflectionsLevel, pMultiTapLS, fReverbMix1 );
			++pChannelLS;
			++pMultiTapLS;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelRS, fWetLevel, fReflectionsLevel, pMultiTapRS, fReverbMix2 );
			++pChannelRS;
			++pMultiTapRS;
			// LFE passthrough
		}
	}
	// Make denormals zero in feedback memories to stop propagation
	LPFAvoidDenormals();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

void CAkReverbFX::ProcessFivePointOne( AkAudioBuffer * io_pBuffer )
{
	assert( m_pDualReverbUnit );

	// Prefetch filter memory
	m_pDualReverbUnit->PrimeCombFiltersCurrent( );
	m_pDualReverbUnit->PrimeAllPassFiltersCurrent( );

	m_GainRamp[OutputLevel].SetTarget( m_pSharedParams->GetMainLevel( ) );
	m_GainRamp[WetDryMix].SetTarget( m_pSharedParams->GetWetDryMix( ) );
	m_GainRamp[ReverbLevel].SetTarget( m_pSharedParams->GetReverbLevel( ) );
	m_GainRamp[EarlyRefLevel].SetTarget( m_pSharedParams->GetReflectionsLevel( ) );
	m_GainRamp[ReverbWidth].SetTarget( m_pSharedParams->GetReverbWidth( ) );
	m_GainRamp[LPFCutFreq].SetTarget( m_pSharedParams->GetCutoffFreq( ) );

	AkReal32 * AK_RESTRICT pChannelL = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTLEFT);		// Left
	AkReal32 * AK_RESTRICT pChannelR = io_pBuffer->GetChannel(AK_IDX_SETUP_5_FRONTRIGHT);		// Right
	AkReal32 * AK_RESTRICT pChannelC = io_pBuffer->GetChannel(AK_IDX_SETUP_5_CENTER);		// Center
	AkReal32 * AK_RESTRICT pChannelLFE = io_pBuffer->GetChannel(AK_IDX_SETUP_5_LFE);		// LFE
	AkReal32 * AK_RESTRICT pChannelLS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARLEFT);		// Left surround
	AkReal32 * AK_RESTRICT pChannelRS = io_pBuffer->GetChannel(AK_IDX_SETUP_5_REARRIGHT);		// Right surround

	AkUInt32 ulNumSampleFrames = io_pBuffer->uValidFrames;
	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;
	if ( bPreStop )
	{
		// Compute length of tail to compute
		AkReal32 fDecayTime = m_pSharedParams->GetDecayTime();
		if ( fDecayTime != m_fPrevDecayTime || m_PrevPreStop == false )
		{
			AkReal32 fTotalReverbTime = fDecayTime + m_pSharedParams->GetReverbDelay() + m_pSharedParams->GetReflectionsDelay();
			m_uSampleFramesToFlush = static_cast<AkUInt32>( fTotalReverbTime * m_uSampleRate );
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

	unsigned long ulFramesToProcess = ulNumSampleFrames;
	while(ulFramesToProcess)
	{
		unsigned long ulFramesToProcessThisLoop = PluginMin(ulFramesToProcess, SUBBLOCK_FRAMESIZE);
		ulFramesToProcess -= ulFramesToProcessThisLoop;

		m_pReflectionsDelay[0].ProcessBuffer( pChannelL, m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[0], ulFramesToProcessThisLoop );
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[2].ProcessBuffer( pChannelC, m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[2].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pReverbDelay[2].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[3].ProcessBuffer( pChannelLS, m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[3].ProcessBuffer( m_pSubBlockBuffer[6], m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pReverbDelay[3].ProcessBuffer( m_pSubBlockBuffer[6], m_pSubBlockBuffer[7], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[4].ProcessBuffer( pChannelRS, m_pSubBlockBuffer[8], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[4].ProcessBuffer( m_pSubBlockBuffer[8], m_pSubBlockBuffer[8], ulFramesToProcessThisLoop );
		m_pReverbDelay[4].ProcessBuffer( m_pSubBlockBuffer[8], m_pSubBlockBuffer[9], ulFramesToProcessThisLoop );

		m_pReflectionsDelay[5].ProcessBuffer( pChannelLFE, m_pSubBlockBuffer[10], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[5].ProcessBuffer( m_pSubBlockBuffer[10], m_pSubBlockBuffer[10], ulFramesToProcessThisLoop );
		m_pReverbDelay[5].ProcessBuffer( m_pSubBlockBuffer[10], m_pSubBlockBuffer[11], ulFramesToProcessThisLoop );

		AddAndMultiply( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], m_pSubBlockBuffer[5],
						m_pSubBlockBuffer[7], m_pSubBlockBuffer[9], m_pSubBlockBuffer[11],
						FIVEPOINTONENORMFACTOR, m_pSubBlockBuffer[1], ulFramesToProcessThisLoop);

		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );

		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[3], m_pSubBlockBuffer[5], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq ); 
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		
		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetDryMix = m_GainRamp[WetDryMix].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fDryLevel = ( REVERB_WETDRYMIX_MAX - fWetDryMix ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		AkReal32 fWetLevel = 1.f - fDryLevel;
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pMultiTapC = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[4];
		AkReal32 * AK_RESTRICT pMultiTapLS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[6];
		AkReal32 * AK_RESTRICT pMultiTapRS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[8];
		AkReal32 * AK_RESTRICT pMultiTapLFE = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[10];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[5];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vDryLevel, vWetLevel, vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2;
		AKSIMD_LOAD1( fDryLevel, vDryLevel );
		AKSIMD_LOAD1( fWetLevel, vWetLevel );
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		AKSIMD_LOAD1( fRevUnitScale1, vUnitScale1Level1 );
		AKSIMD_LOAD1( fRevUnitScale2, vUnitScale1Level2 );
		while ( uBlock4-- )
		{	
			// Cross talk between reverberator units to create stereo spread
			AkReal32Vector vReverbMix1, vReverbMix2, vReverbMix3;
			COMPUTEREVERBUNITMIXSIMDSURROUND( vReverbLevel, pReverbBuf1, pReverbBuf2, vUnitScale1Level1, vUnitScale1Level2, vReverbMix1, vReverbMix2, vReverbMix3 );
			pReverbBuf1+=4;
			pReverbBuf2+=4;

			AkReal32Vector __vDry__, __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelL, vWetLevel, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelR, vWetLevel, vRefLevel, pMultiTapR, vReverbMix2 );
			pChannelR+=4;
			pMultiTapR+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelC, vWetLevel, vRefLevel, pMultiTapC, vReverbMix3 );
			pChannelC+=4;
			pMultiTapC+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelLS, vWetLevel, vRefLevel, pMultiTapLS, vReverbMix1 );
			pChannelLS+=4;
			pMultiTapLS+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelRS, vWetLevel, vRefLevel, pMultiTapRS, vReverbMix2 );
			pChannelRS+=4;
			pMultiTapRS+=4;

			SCALEREVERBCOMPONENTSDUALSIMD( vGain, vDryLevel, pChannelLFE, vWetLevel, vRefLevel, pMultiTapLFE, vReverbMix2 );
			pChannelLFE+=4;
			pMultiTapLFE+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fReverbMix1,fReverbMix2,fReverbMix3,__Unit1__, __Unit2__;
			COMPUTEREVERBUNITMIXSURROUND( fReverbLevel, pReverbBuf1, pReverbBuf2, fRevUnitScale1, fRevUnitScale2, &fReverbMix1, &fReverbMix2, &fReverbMix3 );
			++pReverbBuf1;
			++pReverbBuf2;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelL, fWetLevel, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelR, fWetLevel, fReflectionsLevel, pMultiTapR, fReverbMix2 );
			++pChannelR;
			++pMultiTapR;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelC, fWetLevel, fReflectionsLevel, pMultiTapC, fReverbMix3 );
			++pChannelC;
			++pMultiTapC;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelLS, fWetLevel, fReflectionsLevel, pMultiTapLS, fReverbMix1 );
			++pChannelLS;
			++pMultiTapLS;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelRS, fWetLevel, fReflectionsLevel, pMultiTapRS, fReverbMix2 );
			++pChannelRS;
			++pMultiTapRS;
			SCALEREVERBCOMPONENTSDUAL( fOutGain, fDryLevel, pChannelLFE, fWetLevel, fReflectionsLevel, pMultiTapLFE, fReverbMix2 );
			++pChannelLFE;
			++pMultiTapLFE;
		}
	}
	// Make denormals zero in feedback memories to stop propagation
	LPFAvoidDenormals();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}
//====================================================================================================
