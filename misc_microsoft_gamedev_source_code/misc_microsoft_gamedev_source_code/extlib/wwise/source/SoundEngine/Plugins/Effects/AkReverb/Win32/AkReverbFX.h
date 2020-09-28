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
// AkReverbFX.h
//
// ReverbFX implementation.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_REVERBFX_H_
#define _AK_REVERBFX_H_

#include <AK/Plugin/PluginServices/AkValueRamp.h>
#include "AkReverbFXParams.h"
#include "AkIIROnePoleLPFilter.h"
#include "AkDCFilter.h"
#include "AkReverbUnit.h"
#include "AkDualReverbUnit.h"
#include "AkDelay.h"
#include "AkMultiTapDelay.h"

#include "AkReverbDSP.h"

enum AkReverbLevelRamps
{
	OutputLevel		= 0,
	WetDryMix		= 1,
	ReverbLevel		= 2,
	EarlyRefLevel	= 3,
	ReverbWidth		= 4,
	LPFCutFreq		= 5,
	NumParamRamps	= 6
};

#define MAX_NUM_SUBBLOCK_BUFFERS (2*AK_VOICE_MAX_NUM_CHANNELS)
#define SUBBLOCK_FRAMESIZE (32)

//-----------------------------------------------------------------------------
// Name: class CAkReverbFX
//-----------------------------------------------------------------------------
class CAkReverbFX : public AK::IAkEffectPlugin
{
public:
    
    AK_USE_PLUGIN_ALLOCATOR()

    // Constructor/destructor
    CAkReverbFX();
    ~CAkReverbFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
				   AK::IAkEffectPluginContext *	in_pFXContext,	// FX context interface.
                   AK::IAkPluginParam * in_pParams,				// Effect parameters.
                   AkAudioFormat &		in_rFormat				// Required audio input format.
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

	AkForceInline void CAkReverbFX::SetLPFCutoffFrequency( AkReal32 in_fCutFreq )
	{
		if ( in_fCutFreq != m_fCachedLPCutoffFreq )
		{
			for ( unsigned int i = 0; i < m_uNumLPFilters; ++i )
			{
				m_pLPFilter[i].SetCutoffFrequency( in_fCutFreq );
			}
			m_fCachedLPCutoffFreq = in_fCutFreq;
		}
	}

	AkForceInline void CAkReverbFX::ResetLPFilters( )
	{
		for ( unsigned int i = 0; i < m_uNumLPFilters; ++i )
		{
			m_pLPFilter[i].Reset( );
		}
	}

	AkForceInline void CAkReverbFX::LPFAvoidDenormals( )
	{
		for ( unsigned int i = 0; i < m_uNumLPFilters; ++i )
		{
			m_pLPFilter[i].AvoidDenormal( );
		}
	}

private:

	// Shared parameter interface
    CAkReverbFXParams * m_pSharedParams;

	// Signal processing elements
	CAkReverbUnit * m_pReverbUnit;				// for mono only
	CAkDualReverbUnit * m_pDualReverbUnit;		// 2 for width effect when numchannels >= 2
	CAkOnePoleLPFilter * m_pLPFilter;			// Low pass filters 
	CAkDCFilter m_DCFilter;						// DC offset removal
	
	CAkDelay * m_pReverbDelay;					// 1 per channel
	CAkDelay * m_pReflectionsDelay;				// 1 per channel
	CAkMultiTapDelay * m_pMultiTapDelay;		// 1 per channel

	// Temporary processing buffers
	AkReal32 * m_pSubBlockBuffer[ MAX_NUM_SUBBLOCK_BUFFERS ];

	// Cached values
	AkReal32 m_fCachedReverbTime;	
	AkReal32 m_fCachedLPCutoffFreq;

	// Function ptr to the appropriate DSP execution routine
	void (CAkReverbFX::*m_fpPerformDSP) ( AkAudioBuffer * io_pBuffer );

	// Gain ramp interpolators for Output level, Wet level and Dry level, early reflections and reverb levels
	AK::CAkValueRamp m_GainRamp[NumParamRamps];

	// Audio format information
	AkUInt32 m_uSampleRate;
	AkUInt32 m_uNumChannels;
	AkUInt32 m_uNumLPFilters;

	// FX tail related values
	AkUInt32	m_uSampleFramesToFlush;
	AkReal32	m_fPrevDecayTime;
	bool		m_PrevPreStop;
};

#endif // _AK_REVERBFX_H_