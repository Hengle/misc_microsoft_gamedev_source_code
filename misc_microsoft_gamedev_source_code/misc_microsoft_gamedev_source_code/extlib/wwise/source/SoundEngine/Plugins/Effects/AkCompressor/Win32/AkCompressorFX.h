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
// AkCompressorFX.h
//
// Compressor processing FX implementation.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_COMPRESSORFX_H_
#define _AK_COMPRESSORFX_H_

#include "AkCompressorFXParams.h"

struct AkCompressorSideChain
{
	AkReal32 fGainDb;
	AkReal32 fMem;
};

//-----------------------------------------------------------------------------
// Name: class CAkCompressorFX
//-----------------------------------------------------------------------------
class CAkCompressorFX : public AK::IAkEffectPlugin
{
public:
    
    AK_USE_PLUGIN_ALLOCATOR()

    // Constructor/destructor
    CAkCompressorFX();
    ~CAkCompressorFX();

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
    void Execute( AkAudioBuffer * io_pBuffer );

private:

	void Process( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );
	void ProcessLinked( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );
	void ProcessGain( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain );
	void ProcessGainInt( AkAudioBuffer * io_pBufferIn, AkReal32 fTargetGain );

private:

	// Shared parameter interface
    CAkCompressorFXParams * m_pSharedParams;

	// Function ptr to the appropriate DSP execution routine
	void (CAkCompressorFX::*m_fpPerformDSP)  ( AkAudioBuffer * io_pBufferIn, AkCompressorFXParams * in_pParams );

	AkReal32	m_fCurrentGain;			// Current gain value	
	
	// Audio format information
	AkUInt32	m_uNumChannels;
	AkUInt32	m_uSampleRate;
	
	// Side chain
	AkUInt32	m_uNumSideChain;
	AkReal32	m_fRMSFilterCoef;			// RMS filter coefficient (same for all sidechains)
	AkCompressorSideChain *	m_pSideChain;	// RMS evaluation sidechain

	// Cached values for optimization
	AkReal32 m_fCachedAttack;
	AkReal32 m_fCachedAttackCoef;
	AkReal32 m_fCachedRelease;
	AkReal32 m_fCachedReleaseCoef;

	bool 	m_bProcessLFE;				// LFE behavior
};

#endif // _AK_COMPRESSORFX_H_