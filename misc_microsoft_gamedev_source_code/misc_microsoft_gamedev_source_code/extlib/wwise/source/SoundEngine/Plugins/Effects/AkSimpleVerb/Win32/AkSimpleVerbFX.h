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
// AkSimpleReverbFX.h
//
// SimpleVerbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SIMPLEVERBFX_H_
#define _AK_SIMPLEVERBFX_H_

#include <AK/Plugin/PluginServices/AkValueRamp.h>
#include "AkSimpleVerbFXParams.h"
#include "AkIIROnePoleLPFilter.h"
#include "AkDCFilter.h"
#include "AkReverbUnit.h"

#include "AkSimpleVerbDSP.h"
enum AkSimpleVerbRamps
{
	OutputLevel		= 0,
	WetDryMix		= 1,
	LPFCutFreq		= 2,
	NumParamRamps	= 3
};

#define MAX_NUM_SUBBLOCK_BUFFERS (2)
#define SUBBLOCK_FRAMESIZE (32)

//-----------------------------------------------------------------------------
// Name: class CAkSimpleVerbFX
//-----------------------------------------------------------------------------
class CAkSimpleVerbFX : public AK::IAkEffectPlugin
{
public:
    
    AK_USE_PLUGIN_ALLOCATOR();

    // Constructor/destructor
    CAkSimpleVerbFX();
    ~CAkSimpleVerbFX();

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
	
	// DSP perform routines
	void ProcessMono( AkAudioBuffer * io_pBuffer );	
	void ProcessStereo( AkAudioBuffer * io_pBuffer );	
	void ProcessFivePointZero( AkAudioBuffer * io_pBuffer ); // LFE passthrough
	void ProcessFivePointOne( AkAudioBuffer * io_pBuffer ); // Reverberated LFE

	// Sent Mode versions using optimized hard coded wet/dry values
	void ProcessMonoSent( AkAudioBuffer * io_pBuffer );	
	void ProcessStereoSent( AkAudioBuffer * io_pBuffer );	
	void ProcessFivePointZeroSent( AkAudioBuffer * io_pBuffer ); // LFE silenced
	void ProcessFivePointOneSent( AkAudioBuffer * io_pBuffer ); // Reverberated LFE

	// Function ptr to the appropriate DSP execution routine
	void (CAkSimpleVerbFX::*m_fpPerformDSP) ( AkAudioBuffer * io_pBuffer );
	
	// Shared parameter interface
    CAkSimpleVerbFXParams * m_pSharedParams;

	CAkReverbUnit m_ReverbUnit;
	CAkOnePoleLPFilter m_LPFilter;
	CAkDCFilter m_DCFilter;
	
	AkReal32 * m_pSubBlockBuffer[ MAX_NUM_SUBBLOCK_BUFFERS ];
	AkUInt32 m_uSampleRate;
	AkReal32 m_fCachedReverbTime;	
	AkReal32 m_fCachedLPCutoffFreq;

	// Gain ramp interpolators for Output level, Wet/Dry levels, LPF gain
	AK::CAkValueRamp m_GainRamp[NumParamRamps];

	// FX tail related values
	AkUInt32	m_uSampleFramesToFlush;
	AkReal32	m_fPrevDecayTime;
	bool		m_PrevPreStop;
};

#endif // _AK_SIMPLEVERBFX_H_