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
// AkReverbFXSent.cpp
//
// ReverbFX Send Mode implementation.
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkReverbFX.h"

void CAkReverbFX::ProcessStereoSent( AkAudioBuffer * io_pBuffer )
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
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], pChannelL, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], pChannelR, ulFramesToProcessThisLoop );

		// fReverbIn = ( fReverbDelayL + fReverbDelayR ) * STEREONORMFACTOR;
		AddAndMultiply( pChannelL, pChannelR, STEREONORMFACTOR, m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );

		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );

		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );

		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[2], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );

		// 1 DC normalized one pole low pass filter inserted in series
		SetLPFCutoffFrequency( fLPFCutFreq );
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );

		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		fOutGain *= fWetLevel; // Combine both muliplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[1];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2;
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

			AkReal32Vector __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelL, vGain, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelR, vGain, vRefLevel, pMultiTapR, vReverbMix2 );
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
			SCALEREVERBCOMPONENTSDUALSENT( pChannelL, fOutGain, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelR, fOutGain, fReflectionsLevel, pMultiTapR, fReverbMix2 );
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

void CAkReverbFX::ProcessFivePointZeroSent( AkAudioBuffer * io_pBuffer )
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
		// no need to zero LFE has it is silenced
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
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], pChannelL, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], pChannelR, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[2].ProcessBuffer( pChannelC, m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[2].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pReverbDelay[2].ProcessBuffer( m_pSubBlockBuffer[2], pChannelC, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[3].ProcessBuffer( pChannelLS, m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[3].ProcessBuffer( m_pSubBlockBuffer[3], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pReverbDelay[3].ProcessBuffer( m_pSubBlockBuffer[3], pChannelLS, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[4].ProcessBuffer( pChannelRS, m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[4].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pReverbDelay[4].ProcessBuffer( m_pSubBlockBuffer[4], pChannelRS, ulFramesToProcessThisLoop );

		AddAndMultiply( pChannelL, pChannelR, pChannelC,
			pChannelLS, pChannelRS, FIVEPOINTZERONORMFACTOR,
			m_pSubBlockBuffer[7], ulFramesToProcessThisLoop);

		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[7], ulFramesToProcessThisLoop );

		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[7], m_pSubBlockBuffer[5], m_pSubBlockBuffer[6], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );
		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq ); 
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );

		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		fOutGain *= fWetLevel; // Combine both muliplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[1];
		AkReal32 * AK_RESTRICT pMultiTapC = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pMultiTapLS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];
		AkReal32 * AK_RESTRICT pMultiTapRS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[4];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[5];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[6];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2, vZero;
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		AKSIMD_LOAD1( fRevUnitScale1, vUnitScale1Level1 );
		AKSIMD_LOAD1( fRevUnitScale2, vUnitScale1Level2 );
		AkReal32 fZero = 0.f;
		AKSIMD_LOAD1( fZero, vZero );
		while ( uBlock4-- )
		{	
			// Cross talk between reverberator units to create stereo spread
			AkReal32Vector vReverbMix1, vReverbMix2, vReverbMix3;
			COMPUTEREVERBUNITMIXSIMDSURROUND( vReverbLevel, pReverbBuf1, pReverbBuf2, vUnitScale1Level1, vUnitScale1Level2, vReverbMix1, vReverbMix2, vReverbMix3 );
			pReverbBuf1+=4;
			pReverbBuf2+=4;

			AkReal32Vector __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelL, vGain, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelR, vGain, vRefLevel, pMultiTapR, vReverbMix2 );
			pChannelR+=4;
			pMultiTapR+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelC, vGain, vRefLevel, pMultiTapC, vReverbMix3 );
			pChannelC+=4;
			pMultiTapC+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelLS, vGain, vRefLevel, pMultiTapLS, vReverbMix1 );
			pChannelLS+=4;
			pMultiTapLS+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelRS, vGain, vRefLevel, pMultiTapRS, vReverbMix2 );
			pChannelRS+=4;
			pMultiTapRS+=4;

			// LFE silence
			AKSIMD_STOREVEC( vZero, pChannelLFE );
			pChannelLFE+=4; 
		}

		// Deal with remaining samples
		while ( uRemaining-- )
		{	
			AkReal32 fReverbMix1,fReverbMix2,fReverbMix3,__Unit1__, __Unit2__;
			COMPUTEREVERBUNITMIXSURROUND( fReverbLevel, pReverbBuf1, pReverbBuf2, fRevUnitScale1, fRevUnitScale2, &fReverbMix1, &fReverbMix2, &fReverbMix3 );
			++pReverbBuf1;
			++pReverbBuf2;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelL, fOutGain, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelR, fOutGain, fReflectionsLevel, pMultiTapR, fReverbMix2 );
			++pChannelR;
			++pMultiTapR;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelC, fOutGain, fReflectionsLevel, pMultiTapC, fReverbMix3 );
			++pChannelC;
			++pMultiTapC;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelLS, fOutGain, fReflectionsLevel, pMultiTapLS, fReverbMix1 );
			++pChannelLS;
			++pMultiTapLS;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelRS, fOutGain, fReflectionsLevel, pMultiTapRS, fReverbMix2 );
			++pChannelRS;
			++pMultiTapRS;
			*pChannelLFE++ = 0.f; // LFE silenced
		}
	}
	// Make denormals zero in feedback memories to stop propagation
	LPFAvoidDenormals();
	m_DCFilter.AvoidDenormal();

	io_pBuffer->uValidFrames = (AkUInt16)ulNumSampleFrames;
	m_PrevPreStop = bPreStop;
}

void CAkReverbFX::ProcessFivePointOneSent( AkAudioBuffer * io_pBuffer )
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
		m_pReverbDelay[0].ProcessBuffer( m_pSubBlockBuffer[0], pChannelL, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[1].ProcessBuffer( pChannelR, m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], m_pSubBlockBuffer[1], ulFramesToProcessThisLoop );
		m_pReverbDelay[1].ProcessBuffer( m_pSubBlockBuffer[1], pChannelR, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[2].ProcessBuffer( pChannelC, m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[2].ProcessBuffer( m_pSubBlockBuffer[2], m_pSubBlockBuffer[2], ulFramesToProcessThisLoop );
		m_pReverbDelay[2].ProcessBuffer( m_pSubBlockBuffer[2], pChannelC, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[3].ProcessBuffer( pChannelLS, m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[3].ProcessBuffer( m_pSubBlockBuffer[3], m_pSubBlockBuffer[3], ulFramesToProcessThisLoop );
		m_pReverbDelay[3].ProcessBuffer( m_pSubBlockBuffer[3], pChannelLS, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[4].ProcessBuffer( pChannelRS, m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[4].ProcessBuffer( m_pSubBlockBuffer[4], m_pSubBlockBuffer[4], ulFramesToProcessThisLoop );
		m_pReverbDelay[4].ProcessBuffer( m_pSubBlockBuffer[4], pChannelRS, ulFramesToProcessThisLoop );

		m_pReflectionsDelay[5].ProcessBuffer( pChannelLFE, m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pMultiTapDelay[5].ProcessBuffer( m_pSubBlockBuffer[5], m_pSubBlockBuffer[5], ulFramesToProcessThisLoop );
		m_pReverbDelay[5].ProcessBuffer( m_pSubBlockBuffer[5], pChannelLFE, ulFramesToProcessThisLoop );

		AddAndMultiply( pChannelL, pChannelR, pChannelC,
						pChannelLS, pChannelRS, pChannelLFE,
						FIVEPOINTONENORMFACTOR,m_pSubBlockBuffer[8], ulFramesToProcessThisLoop);

		// DC filter
		m_DCFilter.ProcessBuffer( m_pSubBlockBuffer[8], ulFramesToProcessThisLoop );

		// Network of comb and allpass filters
		m_pDualReverbUnit->ProcessBuffer( m_pSubBlockBuffer[8], m_pSubBlockBuffer[6], m_pSubBlockBuffer[7], ulFramesToProcessThisLoop, ulFramesToProcess == 0  );
		// 1 DC normalized one pole low pass filter inserted in series
		AkReal32 fLPFCutFreq = m_GainRamp[LPFCutFreq].Tick( );
		SetLPFCutoffFrequency( fLPFCutFreq ); 
		m_pLPFilter[0].ProcessBuffer( m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pLPFilter[1].ProcessBuffer( m_pSubBlockBuffer[6], ulFramesToProcessThisLoop );
		m_pLPFilter[2].ProcessBuffer( m_pSubBlockBuffer[7], ulFramesToProcessThisLoop );
		m_pLPFilter[3].ProcessBuffer( m_pSubBlockBuffer[7], ulFramesToProcessThisLoop );

		// Compute interpolated block levels
		AkReal32 fOutGain = m_GainRamp[OutputLevel].Tick( );
		AkReal32 fReflectionsLevel = m_GainRamp[EarlyRefLevel].Tick( );	
		AkReal32 fReverbLevel = m_GainRamp[ReverbLevel].Tick( );
		AkReal32 fReverbWidth = m_GainRamp[ReverbWidth].Tick( );
		AkReal32 fRevUnitScale1 = ( ( 1.f - fReverbWidth ) * 0.5f );
		AkReal32 fRevUnitScale2 = ( ( fReverbWidth * 0.5f ) + 0.5f );
		AkReal32 fWetLevel = m_GainRamp[WetDryMix].Tick( ) * ONEOVER_REVERB_WETDRYMIX_MAX;
		fOutGain *= fWetLevel; // Combine both muliplications

		// Apply volumes
		AkUInt32 uBlock4 = ulFramesToProcessThisLoop / 4;
		AkUInt32 uRemaining = ulFramesToProcessThisLoop - uBlock4*4;
		AkReal32 * AK_RESTRICT pMultiTapL = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[0];
		AkReal32 * AK_RESTRICT pMultiTapR = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[1];
		AkReal32 * AK_RESTRICT pMultiTapC = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[2];
		AkReal32 * AK_RESTRICT pMultiTapLS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[3];
		AkReal32 * AK_RESTRICT pMultiTapRS = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[4];
		AkReal32 * AK_RESTRICT pMultiTapLFE = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[5];
		AkReal32 * AK_RESTRICT pReverbBuf1 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[6];
		AkReal32 * AK_RESTRICT pReverbBuf2 = (AkReal32 * AK_RESTRICT)m_pSubBlockBuffer[7];

		// Process blocks of 4 with loop unroll
		AkReal32Vector vGain, vRefLevel, vReverbLevel, vUnitScale1Level1, vUnitScale1Level2, vZero;
		AKSIMD_LOAD1( fOutGain, vGain );
		AKSIMD_LOAD1( fReflectionsLevel, vRefLevel );
		AKSIMD_LOAD1( fReverbLevel, vReverbLevel );
		AKSIMD_LOAD1( fRevUnitScale1, vUnitScale1Level1 );
		AKSIMD_LOAD1( fRevUnitScale2, vUnitScale1Level2 );
		AkReal32 fZero = 0.f;
		AKSIMD_LOAD1( fZero, vZero );
		while ( uBlock4-- )
		{	
			// Cross talk between reverberator units to create stereo spread
			AkReal32Vector vReverbMix1, vReverbMix2, vReverbMix3;
			COMPUTEREVERBUNITMIXSIMDSURROUND( vReverbLevel, pReverbBuf1, pReverbBuf2, vUnitScale1Level1, vUnitScale1Level2, vReverbMix1, vReverbMix2, vReverbMix3 );
			pReverbBuf1+=4;
			pReverbBuf2+=4;

			AkReal32Vector __vRef__, __vRev__; 
			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelL, vGain, vRefLevel, pMultiTapL, vReverbMix1 );
			pChannelL+=4;
			pMultiTapL+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelR, vGain, vRefLevel, pMultiTapR, vReverbMix2 );
			pChannelR+=4;
			pMultiTapR+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelC, vGain, vRefLevel, pMultiTapC, vReverbMix3 );
			pChannelC+=4;
			pMultiTapC+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelLS, vGain, vRefLevel, pMultiTapLS, vReverbMix1 );
			pChannelLS+=4;
			pMultiTapLS+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelRS, vGain, vRefLevel, pMultiTapRS, vReverbMix2 );
			pChannelRS+=4;
			pMultiTapRS+=4;

			SCALEREVERBCOMPONENTSDUALSIMDSENT( pChannelLFE, vGain, vRefLevel, pMultiTapLFE, vReverbMix2 );
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
			SCALEREVERBCOMPONENTSDUALSENT( pChannelL, fOutGain, fReflectionsLevel, pMultiTapL, fReverbMix1 );
			++pChannelL;
			++pMultiTapL;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelR, fOutGain, fReflectionsLevel, pMultiTapR, fReverbMix2 );
			++pChannelR;
			++pMultiTapR;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelC, fOutGain, fReflectionsLevel, pMultiTapC, fReverbMix3 );
			++pChannelC;
			++pMultiTapC;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelLS, fOutGain, fReflectionsLevel, pMultiTapLS, fReverbMix1 );
			++pChannelLS;
			++pMultiTapLS;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelRS, fOutGain, fReflectionsLevel, pMultiTapRS, fReverbMix2 );
			++pChannelRS;
			++pMultiTapRS;
			SCALEREVERBCOMPONENTSDUALSENT( pChannelLFE, fOutGain, fReflectionsLevel, pMultiTapLFE, fReverbMix2 );
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
