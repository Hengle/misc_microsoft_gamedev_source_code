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
// AkPeakLimiterFX.h
//
// PeakLimiter processing FX implementation.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PEAKLIMITERFX_H_
#define _AK_PEAKLIMITERFX_H_

#include "../AkPeakLimiterFXParams.h"

static const AkUInt32 MAXCHANNELS = 8;

struct AkPeakLimiterSideChain
{
	AkReal32	fGainDb;				// Current gain envelope value
	AkReal32	fCurrentPeak;			// Current peak in look ahead buffer 
	AkUInt32	uPeakTimer;				// Time before current peak value expires
};

//-----------------------------------------------------------------------------
// Name: class CAkPeakLimiterFX
//-----------------------------------------------------------------------------
class CAkPeakLimiterFX : public AK::IAkEffectPlugin
{
public:
    
    AK_USE_PLUGIN_ALLOCATOR()

    // Constructor/destructor
    CAkPeakLimiterFX();
    ~CAkPeakLimiterFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
				    AK::IAkPluginParam * in_pParams,			// Effect parameters.
                    AkAudioFormat &	in_rFormat					// Required audio input format.
				);
    
	// Free memory used by effect and effect termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset or seek to start (looping).
	AKRESULT Reset( );

    // Effect type query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute( AkAudioBuffer *	io_pBuffer );		// Input/Output audio buffer structure.

private:

	void Process( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams );
	void ProcessLinked( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams );
	void ProcessLinkedNoLFE( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams );
	void ProcessGain( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain, bool bProcessLFE );
	void ProcessGainInt( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain, bool bProcessLFE );

	// Function ptr to the appropriate DSP execution routine
	void (CAkPeakLimiterFX::*m_fpPerformDSP)  ( AkAudioBuffer * io_pBufferIn, AkPeakLimiterFXParams * in_pParams );

private:

	// Shared parameter interface
    CAkPeakLimiterFXParams * m_pSharedParams;
	
	AkReal32	m_fCurrentGain;			// Current gain value			

	// Audio format information
	AkUInt32	m_uNumChannels;
	AkUInt32	m_uNumPeakLimitedChannels;
	AkUInt32	m_uSampleRate;	

	// Sidechain
	AkUInt32	m_uNumSideChain;
	AkUInt32	m_uLookAheadFrames;			// Number of sample frames in look-ahead buffer (all side chains)
	AkPeakLimiterSideChain * m_SideChains;	// Variable number of side chains

	// Channel delays
	AkReal32 *	m_pfDelayBuffer;		// Delay line storage
	AkUInt32	m_uFramePos;			// Current position within delay line

	// Frames remaining in delay buffers (tail flush)
	AkUInt32	m_uFramesRemaining;	
	
	// Cached values for optimization
	AkReal32 m_fCachedRelease;	
	AkReal32 m_fReleaseCoef;
	AkReal32 m_fAttackCoef;	

	bool	m_bFirstTime;				// First time peak calculation, must go through look ahead buffer
	bool	m_bProcessLFE;
};

#endif // _AK_PEAKLIMITERFX_H_