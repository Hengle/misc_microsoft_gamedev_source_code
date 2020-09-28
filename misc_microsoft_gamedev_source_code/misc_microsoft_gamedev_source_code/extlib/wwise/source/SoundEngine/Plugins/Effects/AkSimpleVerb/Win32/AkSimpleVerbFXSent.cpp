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
// AkSimpleVerbFXSent.cpp
//
// SimpleVerbFX Sent Mode implementation.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkSimpleVerbFX.h"



void CAkSimpleVerbFX::ProcessStereoSent( AkAudioBuffer * io_pBuffer )
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

		// Compute interpolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		fCurOutGain *= fWetLevel; // Combine both multiplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain;
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vRev;
			AKSIMD_LOADVEC( pReverbBuf, vRev );
			vRev = AKSIMD_MUL( vRev, vGain );

			AKSIMD_STOREVEC( vRev, pChannelL );
			pChannelL+=4;
			AKSIMD_STOREVEC( vRev, pChannelR );
			pChannelR+=4;

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelL, *pReverbBuf );
			++pChannelL;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelR, *pReverbBuf );
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

// Process 5.1 signals and memset LFE to avoid doubling with dry path. Do not use wet/dry mix values (send mode optimization).
void CAkSimpleVerbFX::ProcessFivePointZeroSent( AkAudioBuffer * io_pBuffer )
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
		// no need to zero LFE has it is silenced
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

		// Compute interpolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		fCurOutGain *= fWetLevel; // Combine both multiplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain, vZero;
		AkReal32 fZero = 0.f;
		AKSIMD_LOAD1( fZero, vZero );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vRev;
			AKSIMD_LOADVEC( pReverbBuf, vRev );
			vRev = AKSIMD_MUL( vRev, vGain );

			AKSIMD_STOREVEC( vRev, pChannelL );
			pChannelL+=4;
			AKSIMD_STOREVEC( vRev, pChannelR );
			pChannelR+=4;
			AKSIMD_STOREVEC( vRev, pChannelC );
			pChannelC+=4;
			AKSIMD_STOREVEC( vRev, pChannelLS );
			pChannelLS+=4;
			AKSIMD_STOREVEC( vRev, pChannelRS );
			pChannelRS+=4;
			// LFE drop because this is sent FX
			AKSIMD_STOREVEC( vZero, pChannelLFE );
			pChannelLFE+=4; 

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelL, *pReverbBuf );
			++pChannelL;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelR, *pReverbBuf );
			++pChannelR;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelC, *pReverbBuf );
			++pChannelC;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelLS, *pReverbBuf );
			++pChannelLS;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelRS, *pReverbBuf );
			++pChannelRS;
			*pChannelLFE++ = 0.f; // LFE drop because this is sent FX

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
// Process 5.1 signals (including LFE). Do not use wet/dry mix values (send mode optimization).
void CAkSimpleVerbFX::ProcessFivePointOneSent( AkAudioBuffer * io_pBuffer )
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

		// fReverbIn = ( fDryL + fDryR + fDryC + fDryLS + fDryRS + fDryLFE) * FIVEPOINTONENORMFACTOR;
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

		// Compute interpolated block levels
		AkReal32 fCurOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_SIMPLEVERBFXPARAM_WETDRYMIX_MAX;
		fCurOutGain *= fWetLevel; // Combine both multiplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;	
		AkReal32 * AK_RESTRICT pReverbBuf = pReverbBuffer;

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain, vZero;
		AkReal32 fZero = 0.f;
		AKSIMD_LOAD1( fZero, vZero );
		AKSIMD_LOAD1( fCurOutGain, vGain );
		while ( uBlock4-- )
		{	
			AkReal32Vector vRev;
			AKSIMD_LOADVEC( pReverbBuf, vRev );
			vRev = AKSIMD_MUL( vRev, vGain );

			AKSIMD_STOREVEC( vRev, pChannelL );
			pChannelL+=4;
			AKSIMD_STOREVEC( vRev, pChannelR );
			pChannelR+=4;
			AKSIMD_STOREVEC( vRev, pChannelC );
			pChannelC+=4;
			AKSIMD_STOREVEC( vRev, pChannelLS );
			pChannelLS+=4;
			AKSIMD_STOREVEC( vRev, pChannelRS );
			pChannelRS+=4;
			AKSIMD_STOREVEC( vRev, pChannelLFE );
			pChannelLFE+=4; 

			pReverbBuf+=4;
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelL, *pReverbBuf );
			++pChannelL;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelR, *pReverbBuf );
			++pChannelR;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelC, *pReverbBuf );
			++pChannelC;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelLS, *pReverbBuf );
			++pChannelLS;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelRS, *pReverbBuf );
			++pChannelRS;
			SCALEREVERBCOMPONENTSSENT( fCurOutGain, pChannelLFE, *pReverbBuf );
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
