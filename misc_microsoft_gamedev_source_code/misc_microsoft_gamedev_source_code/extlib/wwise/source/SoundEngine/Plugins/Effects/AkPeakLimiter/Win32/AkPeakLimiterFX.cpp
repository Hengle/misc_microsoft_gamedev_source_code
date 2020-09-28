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
// AkPeakLimiterFX.cpp
//
// PeakLimiter processing FX implementation.
// 
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkPeakLimiterFX.h"
#include <math.h>
#include <assert.h>
#include "AkDbToLin.h"
#include "AkDSPUtils.h"

static const AkReal32 SMALL_DC_OFFSET = 1.e-25f;	// DC offset to avoid log(0)
// Note: Scaling factor is such that time specified corresponds to time to reach 90% of target value
static const AkReal32 SCALE_RAMP_TIME = 2.2f;		// Correction factor to make attack/release times accurate

// Plugin mechanism. Dynamic create function whose address must be registered to the FX manager.
AK::IAkPlugin* CreatePeakLimiterFX( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkPeakLimiterFX( ) );
}

// Constructor.
CAkPeakLimiterFX::CAkPeakLimiterFX()
{
	m_pSharedParams = NULL;
	m_fpPerformDSP = NULL;
	m_SideChains = NULL;
}

// Destructor.
CAkPeakLimiterFX::~CAkPeakLimiterFX()
{

}

// Initializes and allocate memory for the effect
AKRESULT CAkPeakLimiterFX::Init(	AK::IAkPluginMemAlloc * in_pAllocator,		// Memory allocator interface.
									AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
									AK::IAkPluginParam * in_pParams,			// Effect parameters.
									AkAudioFormat &	in_rFormat					// Required audio input format.
								)
{
	// Set parameters interface and retrieve init params.
	m_pSharedParams = reinterpret_cast<CAkPeakLimiterFXParams*>(in_pParams);
	AkPeakLimiterFXParams Params;
	m_pSharedParams->GetParams( &Params );

	// Should not be able to change those at run-time (Wwise only problem)
	m_bProcessLFE = Params.bProcessLFE;
	m_uNumPeakLimitedChannels = m_uNumChannels = in_rFormat.GetNumChannels( );
	// Gain and peak limiting is not applied to LFE channel
	if ( !Params.bProcessLFE && in_rFormat.HasLFE() )
		--m_uNumPeakLimitedChannels;

	if ( Params.bChannelLink )
		m_uNumSideChain = 1;
	else
	{
		// No need to have side chain for LFE if we are not processing it (still delayed however)
		m_uNumSideChain = m_uNumPeakLimitedChannels;
	}

	m_uSampleRate = in_rFormat.uSampleRate;
	m_uLookAheadFrames = static_cast<AkUInt32>( Params.fLookAhead * m_uSampleRate );
	
	// Initialized cached values
	// Note: Attack time is hard coded to half the look ahead time
	m_fAttackCoef = exp( -SCALE_RAMP_TIME / ( m_uLookAheadFrames/2.f ) );
	m_fCachedRelease = Params.fRelease;
	m_fReleaseCoef = exp( -SCALE_RAMP_TIME / ( Params.fRelease * m_uSampleRate ) );

	// Note: LFE channels is always delayed as well
	m_pfDelayBuffer = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32)*m_uLookAheadFrames*m_uNumChannels );	
	if ( m_pfDelayBuffer == NULL )
		return AK_InsufficientMemory;
	m_uFramePos = 0;

	// Note: Mono case can be handled by both routines, faster with unlinked process	
	if ( !Params.bChannelLink || in_rFormat.GetChannelMask() == AK_SPEAKER_SETUP_MONO )
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::Process;
	}
	else if ( !in_rFormat.HasLFE() || Params.bProcessLFE ) // bChannelLink == true
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::ProcessLinked;
	}
	else // bChannelLink == true && in_rFormat.HasLFE() == true && Params.bProcessLFE == false
	{
		m_fpPerformDSP = &CAkPeakLimiterFX::ProcessLinkedNoLFE; // Delay LFE without peak limiting it
	}

	// Side chains alloc	
	if ( m_uNumSideChain )
	{
		m_SideChains = (AkPeakLimiterSideChain*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkPeakLimiterSideChain)*m_uNumSideChain );	
		if ( m_SideChains == NULL )
			return AK_InsufficientMemory;
	}

	// Gain ramp initialization for Output level
	m_fCurrentGain = Params.fOutputLevel;

	// Tail flush
	m_uFramesRemaining = m_uLookAheadFrames;

	return AK_Success;
}

// Deallocates and kill effect instance.
AKRESULT CAkPeakLimiterFX::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	if ( m_pfDelayBuffer )
		AK_PLUGIN_FREE( in_pAllocator, m_pfDelayBuffer );	

	if ( m_SideChains )
		AK_PLUGIN_FREE( in_pAllocator, m_SideChains );

	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Clear peak hold, delay lines and side chain states
AKRESULT CAkPeakLimiterFX::Reset( )
{
	memset( m_pfDelayBuffer, 0, sizeof(AkReal32)*m_uLookAheadFrames*m_uNumChannels );	
	if ( m_SideChains )
	{
		for ( AkUInt32 i = 0; i < m_uNumSideChain; ++i )
		{
			m_SideChains[i].fCurrentPeak = 0.f;
			m_SideChains[i].fGainDb = 0.f;
			m_SideChains[i].uPeakTimer = 0;
		}
	}

	// Go through next buffer to find peak
	m_bFirstTime = true;

	return AK_Success;
}

// Effect info query.
AKRESULT CAkPeakLimiterFX::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
	out_rPluginInfo.eType = AkPluginTypeEffect;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Execute dynamics processing effect.
//-----------------------------------------------------------------------------
void CAkPeakLimiterFX::Execute(	AkAudioBuffer* io_pBuffer )
{
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

	AkPeakLimiterFXParams Params;
	m_pSharedParams->GetParams( &Params );

	// Don't allow those to change at run-time
	Params.bProcessLFE = m_bProcessLFE;

	if ( Params.fRelease != m_fCachedRelease )
	{
		m_fCachedRelease = Params.fRelease;
		m_fReleaseCoef = exp( -SCALE_RAMP_TIME / ( Params.fRelease * m_uSampleRate ) );
	}
	// Change the attack and release time to hold directly the coefficient because that's all DSP needs to know
	Params.fRelease = m_fReleaseCoef;
	Params.fLookAhead = m_fAttackCoef;

	bool bPreStop = io_pBuffer->eState == AK_NoMoreData;	
	if ( bPreStop )
	{
		// Note: Sound engine guarantees 0s between validFrames and MaxFrames so that the effect processing can continue
		// unchanged to produce the tail receiving silenced input.
		AkUInt32 uTailFrames = PluginMin( m_uFramesRemaining, (AkUInt32)(io_pBuffer->MaxFrames()-io_pBuffer->uValidFrames) );
		// Zero out invalid frames between uValidFrames and MaxFrames() (make it valid before we will use it).
		// By just feeding in zeroes to the delay, we can keep the same algorithm regardless if we are in tail or not.
		for ( AkUInt32 i = 0; i < m_uNumChannels; ++i )
		{
			AkReal32 * AK_RESTRICT pPadStart = io_pBuffer->GetChannel(i) + io_pBuffer->uValidFrames;
			AKPLATFORM::AkMemSet( pPadStart, 0, uTailFrames*sizeof(AkReal32) );
		}	
		io_pBuffer->uValidFrames += (AkUInt16)uTailFrames;
		m_uFramesRemaining -= uTailFrames;
		if ( m_uFramesRemaining > 0 )
			io_pBuffer->eState = AK_DataReady;
	}
	else
	{
		// Possible to get out of tail mode on Bus
		m_uFramesRemaining = m_uLookAheadFrames;
	}

	(this->*m_fpPerformDSP)( io_pBuffer, &Params );

	// Apply output gain
	AkReal32 fTargetGain = Params.fOutputLevel;
	if ( fTargetGain != m_fCurrentGain )
	{
		ProcessGainInt( io_pBuffer, fTargetGain, m_bProcessLFE );
		m_fCurrentGain = fTargetGain;
	}
	else
	{
		ProcessGain( io_pBuffer, fTargetGain, m_bProcessLFE );
	}
	
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
}

void CAkPeakLimiterFX::ProcessGainInt( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain, bool bProcessLFE )
{	
	AkReal32 fGainInc = (fTargetGain - m_fCurrentGain) / io_pBufferIn->MaxFrames();

	// Skip LFE if necessary (LFE is always the last channel).
	AkUInt32 uNumChannels = m_uNumPeakLimitedChannels;
	for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
	{
		AkReal32 fCurrentGain = m_fCurrentGain;
		register AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(uChan);	
		register AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		while ( pfBuf < pfEnd )
		{
			fCurrentGain += fGainInc;
			*pfBuf = fCurrentGain * *pfBuf;
			++pfBuf;
		}
	}
}

void CAkPeakLimiterFX::ProcessGain( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain, bool bProcessLFE )
{
	// Skip LFE if necessary (LFE is always the last channel).
	AkUInt32 uNumChannels = m_uNumPeakLimitedChannels;
	for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
	{
		AkReal32 fCurrentGain = m_fCurrentGain;
		register AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(uChan);	
		register AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		while ( pfBuf < pfEnd )
		{
			*pfBuf = fCurrentGain * *pfBuf;
			++pfBuf;
		}
	}
}

// Note: delay line buffer is written DEINTERLEAVED in this mode of processing
void CAkPeakLimiterFX::Process( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams )
{
	AkReal32 fThresh = in_pParams->fThreshold;
	AkReal32 fRatioFactor = (1.f / in_pParams->fRatio) - 1.f;
	AkReal32 fAttackCoef = in_pParams->fLookAhead;
	AkReal32 fReleaseCoef = in_pParams->fRelease; // Release params here contains the coefficient directly
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = NULL;
	AkReal32 * AK_RESTRICT pfDelayBuffer = NULL;
	AkUInt32 uNumFrames = io_pBufferIn->uValidFrames;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uFramePos = m_uFramePos;

	AkUInt32 uNumPeakLimitedChannels = m_uNumPeakLimitedChannels;
	
	// Just delay LFE if necessary
	if ( !in_pParams->bProcessLFE && io_pBufferIn->HasLFE() )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetLFE();
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		AkUInt32 uChannelOffset = uNumPeakLimitedChannels*uLookAheadFrames;
		pfDelayBufferRWPtr = m_pfDelayBuffer + uChannelOffset + uFramePos;
		const AkReal32 * AK_RESTRICT pfDelayBufferEnd = m_pfDelayBuffer + uChannelOffset + uLookAheadFrames;
		pfDelayBuffer = m_pfDelayBuffer + uChannelOffset;

		while ( pfBuf < pfEnd )
		{
			AkUInt32 uFramesBeforeBoundary = (AkUInt32)PluginMin( pfEnd - pfBuf, pfDelayBufferEnd - pfDelayBufferRWPtr );
			while ( uFramesBeforeBoundary-- )
			{
				// Read delay sample
				AkReal32 fDelayedSample = *pfDelayBufferRWPtr;
				// Write new input to delay line
				*pfDelayBufferRWPtr++ = *pfBuf;				
				// Output delayed LFE
				*pfBuf++ = fDelayedSample;
			}
			// Wrap delay line
			if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
				pfDelayBufferRWPtr = pfDelayBuffer;
		}
	}

	// Process all channels (except LFE if it was handled above).
	for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
	{
		AkReal32 * AK_RESTRICT pfBuf = io_pBufferIn->GetChannel(uChan);	
		AkReal32 * AK_RESTRICT pfEnd = pfBuf + io_pBufferIn->uValidFrames;

		AkUInt32 uChannelOffset = uChan*uLookAheadFrames;
		pfDelayBufferRWPtr = m_pfDelayBuffer + uChannelOffset + uFramePos;
		const AkReal32 * AK_RESTRICT pfDelayBufferEnd = m_pfDelayBuffer + uChannelOffset + uLookAheadFrames;
		pfDelayBuffer = m_pfDelayBuffer + uChannelOffset;

		// Local variables for performance
		AkReal32 fLocGainDb = m_SideChains[uChan].fGainDb;
		AkReal32 fCurrentPeak = m_SideChains[uChan].fCurrentPeak;
		AkUInt32 uPeakTimer = m_SideChains[uChan].uPeakTimer;

		// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
		if ( m_bFirstTime )
		{
			AkUInt32 uLoopFrames = PluginMin( uLookAheadFrames, io_pBufferIn->uValidFrames );
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pfBuf[i]);
				if ( fAbsX >= fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}
			if ( uChan == uNumPeakLimitedChannels-1 )
				m_bFirstTime = false;
		}

		while ( pfBuf < pfEnd )
		{
			AkUInt32 uFramesBeforeBoundary = (AkUInt32)PluginMin( pfEnd - pfBuf, pfDelayBufferEnd - pfDelayBufferRWPtr );
			while ( uFramesBeforeBoundary-- )
			{
				// Read delay sample
				AkReal32 fDelayedSample = *pfDelayBufferRWPtr;
				// Write new input to delay line
				AkReal32 fIn = *pfBuf;
				*pfDelayBufferRWPtr++ = fIn;
				
				// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
				// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
				// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
				// related to this.

				// Get a new peak value if higher than current or if old peak has timed out.
				fIn = fabs( fIn );	// |x[n]| (rectification)		
				if ( !uPeakTimer || fIn > fCurrentPeak )
				{
					fCurrentPeak = fIn;					// New peak value
					uPeakTimer = uLookAheadFrames;		// Reset timer
				}
				else
					uPeakTimer--;

				// Find dB over threshold
				AkReal32 fPowerDb = 20.f*AK::FastLog10( fCurrentPeak );	// Convert power estimation to dB
				AkReal32 fDbOverThresh = fPowerDb - fThresh;			// Offset into non-linear range (over threshold)
				fDbOverThresh = PluginFPMax( fDbOverThresh, 0.f );

				// Attack and release smoothing
				AkReal32 fCoef = PluginFPSel(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
				fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

				// Static transfer function evaluation
				AkReal32 fGainReductiondB = fLocGainDb * fRatioFactor;				// Gain reduction (dB)
				AkReal32 fGainReduction = AK::dBToLin( fGainReductiondB );			// Convert to linear

				// Apply compression gain
				*pfBuf++ = fDelayedSample * fGainReduction;
			}
			// Wrap delay line
			if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
				pfDelayBufferRWPtr = pfDelayBuffer;
		}
		
		// Local variables for performance
		m_SideChains[uChan].fGainDb = fLocGainDb;
		m_SideChains[uChan].fCurrentPeak = fCurrentPeak;
		m_SideChains[uChan].uPeakTimer = uPeakTimer;
	}

	// Update frame position within delay lines
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - pfDelayBuffer);
	assert( m_uFramePos <= m_uLookAheadFrames );
}

// Note: delay line buffer is written INTERLEAVED in this mode of processing
// Note: The LFE (if present) is not treated differently than other channels in this routine 
void CAkPeakLimiterFX::ProcessLinked( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams )
{
	// Note assuming all channels are the same threshold needs to be scaled by sqrt(numChannels) when adding power of
	// many channels
	AkReal32 fThresh = in_pParams->fThreshold;
	AkReal32 fRatioFactor = (1.f / in_pParams->fRatio) - 1.f;
	AkReal32 fAttackCoef = in_pParams->fLookAhead;
	AkReal32 fReleaseCoef = in_pParams->fRelease;

	// Local variables for performance
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = m_pfDelayBuffer + m_uFramePos*m_uNumChannels;
	const AkReal32 * pfDelayBufferEnd = m_pfDelayBuffer + m_uLookAheadFrames*m_uNumChannels;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uNumFrames = io_pBufferIn->uValidFrames;
	AkReal32 fLocGainDb = m_SideChains->fGainDb;
	AkReal32 fCurrentPeak = m_SideChains->fCurrentPeak;
	AkUInt32 uPeakTimer = m_SideChains->uPeakTimer;

	// Setup pointers to all channels
	AkUInt32 uNumChannels = m_uNumChannels;
	AkReal32 * AK_RESTRICT pfBuf[MAXCHANNELS];
	for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
	{
		pfBuf[uChan]= io_pBufferIn->GetChannel(uChan);	
	}
	AkReal32 fDelayedSamples[MAXCHANNELS];

	// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
	if ( m_bFirstTime )
	{
		AkUInt32 uLoopFrames = PluginMin( uLookAheadFrames, io_pBufferIn->uValidFrames );
		for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
		{
			AkReal32 * AK_RESTRICT pChanBuf = pfBuf[uChan];
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pChanBuf[i]);
				if ( fAbsX > fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}			
		}
		m_bFirstTime = false;
	}

	AkUInt32 uIndex = 0;
	while ( uIndex < uNumFrames )
	{
		AkUInt32 uFramesBeforeBoundary = (AkUInt32)PluginMin( uNumFrames-uIndex, (pfDelayBufferEnd - pfDelayBufferRWPtr) / uNumChannels );

		while ( uFramesBeforeBoundary-- )
		{
			// Find peak value in all channels
			AkReal32 fChannelsPeak = 0.f;
			for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
			{
				// Read delay samples to local storage
				fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
				// Write new input to delay lines
				AkReal32 fIn = pfBuf[uChan][uIndex];
				pfDelayBufferRWPtr[uChan] = fIn;
				fIn = fabs( fIn );	// |x[n]| (rectification)	
				fChannelsPeak = PluginFPMax( fIn, fChannelsPeak );			
			}
			pfDelayBufferRWPtr += uNumChannels;

			// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
			// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
			// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
			// related to this.

			// Get a new peak value if higher than current or if old peak has timed out.
			if ( !uPeakTimer || fChannelsPeak > fCurrentPeak )
			{
				fCurrentPeak = fChannelsPeak;		// New peak value
				uPeakTimer = uLookAheadFrames;		// Reset timer
			}
			else
				uPeakTimer--;

			// Find dB over threshold
			AkReal32 fPowerDb = 20.f*log10( fCurrentPeak );		// Convert power estimation to dB
			AkReal32 fDbOverThresh = fPowerDb - fThresh;		// Offset into non-linear range (over threshold)
			fDbOverThresh = PluginFPMax( fDbOverThresh, 0.f );

			// Attack and release smoothing
			AkReal32 fCoef = PluginFPSel(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
			fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

			// Static transfer function evaluation
			AkReal32 fGainReductiondB = fLocGainDb * fRatioFactor;				// Gain reduction (dB)
			AkReal32 fGainReduction = AK::dBToLin( fGainReductiondB );			// Convert to linear

			// Apply compression gain to all channels
			for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
			{
				pfBuf[uChan][uIndex] = fDelayedSamples[uChan] * fGainReduction;
			}
			++uIndex;
		}

		if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
			pfDelayBufferRWPtr = m_pfDelayBuffer;
	}

	// Save local variables
	m_SideChains->fGainDb = fLocGainDb;
	m_SideChains->fCurrentPeak = fCurrentPeak;
	m_SideChains->uPeakTimer = uPeakTimer;
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - m_pfDelayBuffer) / m_uNumChannels;
	assert( m_uFramePos <= m_uLookAheadFrames );
}

// This routine is called when the processing is linked AND there is a LFE present that must not be peak limited but only delayed
// Note: delay line buffer is written INTERLEAVED in this mode of processing
void CAkPeakLimiterFX::ProcessLinkedNoLFE( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams )
{
	// Note assuming all channels are the same threshold needs to be scaled by sqrt(numChannels) when adding power of
	// many channels
	AkReal32 fThresh = in_pParams->fThreshold;
	AkReal32 fRatioFactor = (1.f / in_pParams->fRatio) - 1.f;
	AkReal32 fAttackCoef = in_pParams->fLookAhead;
	AkReal32 fReleaseCoef = in_pParams->fRelease;
	AKRESULT eResult = AK_DataReady;

	// Local variables for performance
	AkReal32 * AK_RESTRICT pfDelayBufferRWPtr = m_pfDelayBuffer + m_uFramePos*m_uNumChannels;
	const AkReal32 * pfDelayBufferEnd = m_pfDelayBuffer + m_uLookAheadFrames*m_uNumChannels;
	AkUInt32 uLookAheadFrames = m_uLookAheadFrames;
	AkUInt32 uNumFrames = io_pBufferIn->uValidFrames;
	AkReal32 fLocGainDb = m_SideChains->fGainDb;
	AkReal32 fCurrentPeak = m_SideChains->fCurrentPeak;
	AkUInt32 uPeakTimer = m_SideChains->uPeakTimer;
	const AkUInt32 uNumPeakLimitedChannels = m_uNumPeakLimitedChannels;

	// Setup pointers to all channels
	assert( io_pBufferIn->HasLFE() );
	const AkUInt32 uNumChannels = m_uNumChannels;
	AkReal32 * AK_RESTRICT pfBuf[MAXCHANNELS];
	for ( AkUInt32 uChan = 0; uChan < uNumChannels; ++uChan )
	{
		pfBuf[uChan]= io_pBufferIn->GetChannel(uChan);	
	}
	AkReal32 fDelayedSamples[MAXCHANNELS];

	// If first buffer received, process a whole buffer to find peak ( reset() reinitializes this flag)
	if ( m_bFirstTime )
	{
		AkUInt32 uLoopFrames = PluginMin( uLookAheadFrames, io_pBufferIn->uValidFrames );

		// Skip LFE in this loop (we know that it is present and that it is the last channel).
		for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
		{
			AkReal32 * AK_RESTRICT pChanBuf = pfBuf[uChan];
			for ( AkUInt32 i = 0; i < uLoopFrames; ++i )
			{
				AkReal32 fAbsX = fabs( pChanBuf[i]);
				if ( fAbsX > fCurrentPeak )
				{
					fCurrentPeak = fAbsX;
					uPeakTimer = uLoopFrames - i;
				}
			}			
		}
		m_bFirstTime = false;
	}

	AkUInt32 uIndex = 0;
	while ( uIndex < uNumFrames )
	{
		AkUInt32 uFramesBeforeBoundary = (AkUInt32)PluginMin( uNumFrames-uIndex, (pfDelayBufferEnd - pfDelayBufferRWPtr) / uNumChannels );

		while ( uFramesBeforeBoundary-- )
		{
			// Find peak value in all processed channels
			AkReal32 fChannelsPeak = 0.f;
			{
				AkUInt32 uChan = 0;
				for ( ; uChan < uNumPeakLimitedChannels; ++uChan )
				{
					// Read delay samples to local storage
					fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
					// Write new input to delay lines
					AkReal32 fIn = pfBuf[uChan][uIndex];
					pfDelayBufferRWPtr[uChan] = fIn;
					fIn = fabs( fIn );	// |x[n]| (rectification)	
					fChannelsPeak = PluginFPMax( fIn, fChannelsPeak );		
				}

				// Handle LFE channel separately
				fDelayedSamples[uChan] = pfDelayBufferRWPtr[uChan];
				AkReal32 fIn = pfBuf[uChan][uIndex];
				pfDelayBufferRWPtr[uChan] = fIn;
				pfBuf[uChan][uIndex] = fDelayedSamples[uChan];

				pfDelayBufferRWPtr += uNumChannels;
			}

			// IMPORTANT NOTE: We know that simply getting next value as the next peak once the old one times out is not ideal.
			// It most likely will not be the highest sample of the lookahead buffer. This on the other hand allows
			// tremendous memory and CPU optimizations. In practice release times used are long enough to avoid problems
			// related to this.

			// Get a new peak value if higher than current or if old peak has timed out.
			if ( !uPeakTimer || fChannelsPeak > fCurrentPeak )
			{
				fCurrentPeak = fChannelsPeak;		// New peak value
				uPeakTimer = uLookAheadFrames;		// Reset timer
			}
			else
				uPeakTimer--;

			// Find dB over threshold
			AkReal32 fPowerDb = 20.f*log10( fCurrentPeak );		// Convert power estimation to dB
			AkReal32 fDbOverThresh = fPowerDb - fThresh;		// Offset into non-linear range (over threshold)
			fDbOverThresh = PluginFPMax( fDbOverThresh, 0.f );

			// Attack and release smoothing
			AkReal32 fCoef = PluginFPSel(fDbOverThresh - fLocGainDb, fAttackCoef, fReleaseCoef );
			fLocGainDb = fDbOverThresh + fCoef * ( fLocGainDb - fDbOverThresh );

			// Static transfer function evaluation
			AkReal32 fGainReductiondB = fLocGainDb * fRatioFactor;				// Gain reduction (dB)
			AkReal32 fGainReduction = AK::dBToLin( fGainReductiondB );			// Convert to linear

			// Apply compression gain to all processed channels
			for ( AkUInt32 uChan = 0; uChan < uNumPeakLimitedChannels; ++uChan )
			{
				pfBuf[uChan][uIndex] = fDelayedSamples[uChan] * fGainReduction;
			}
			++uIndex;
		}

		if ( pfDelayBufferRWPtr == pfDelayBufferEnd )
			pfDelayBufferRWPtr = m_pfDelayBuffer;
	}

	// Save local variables
	m_SideChains->fGainDb = fLocGainDb;
	m_SideChains->fCurrentPeak = fCurrentPeak;
	m_SideChains->uPeakTimer = uPeakTimer;
	m_uFramePos = (AkUInt32) (pfDelayBufferRWPtr - m_pfDelayBuffer) / m_uNumChannels;
	assert( m_uFramePos <= m_uLookAheadFrames );
}
