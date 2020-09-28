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
// AkReverbFXParams.h
//
// Shared parameter implementation for reverb FX.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_REVERBFXPARAMS_H_
#define _AK_REVERBFXPARAMS_H_

#include "AkCommonReverbFXParams.h"

#include <AK/Plugin/AkReverbFXFactory.h>
#include <math.h>
#include <assert.h>
#include "AkDbToLin.h"

//-----------------------------------------------------------------------------
// Structures.
//-----------------------------------------------------------------------------

// Structure of reverbFX parameters
struct AkReverbFXParams
{
	AkReal32		fWetDryMix;				// Wet/dry ratio (0.,-100.)
	AkReal32		fReflectionsLevel;		// Level for early reflections (-96.3,0) in dB
	AkReal32		fReverbLevel;			// Level for reverb tail (-96.3,0) in dB			
	AkReal32		fReflectionsDelay;		// Time prior to early reflections (secs)
	AkReal32		fReverbDelay;			// Time between early reflections and reverb tail
	AkReal32		fDecayTime;				// Decay time of reverb tail (secs)
	AkReal32		fCutoffFreq; 			// LPF cutoff frequency
	AkReal32		fReverbWidth;			// Reverb stereo width (stereo only)
	AkReal32		fMainLevel;				// Main gain for wet+dry output sound
	bool			bProcessLFE;
} AK_ALIGNED_16;

//-----------------------------------------------------------------------------
// Name: class CAkReverbFXParams
// Desc: Shared reverb FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkReverbFXParams : public AK::IAkPluginParam
{
public:

    AK_USE_PLUGIN_ALLOCATOR();

	friend class CAkReverbFX;
    
    // Constructor/destructor.
    CAkReverbFXParams( );
    ~CAkReverbFXParams();
	CAkReverbFXParams( const CAkReverbFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,						    
                    void *					in_pParamsBlock, 
                    AkUInt32				in_ulBlockSize 
                         );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock(	void * in_pParamsBlock, 
                                AkUInt32 in_ulBlockSize
                                );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
                        void * in_pValue, 
                        AkUInt32 in_ulParamSize
                        );

private:
	void GetParams(AkReverbFXParams* in_pParams)
	{
		LockParams();
		*in_pParams = m_Params;
		UnlockParams();
	}

	inline AkReal32	GetWetDryMix();
	inline AkReal32	GetReflectionsLevel();
	inline AkReal32	GetReverbLevel();
	inline AkReal32	GetReflectionsDelay();
	inline AkReal32	GetReverbDelay();
	inline AkReal32	GetDecayTime();
	inline AkReal32	GetCutoffFreq();
	inline AkReal32	GetReverbWidth();
	inline AkReal32	GetMainLevel();
	inline bool		GetProcessLFE();

    // Parameter blob.
    AkReverbFXParams	m_Params;
};

// GetWetDryMix
inline AkReal32 CAkReverbFXParams::GetWetDryMix( )
{
	LockParams( );
	AkReal32 fWetDryMix = m_Params.fWetDryMix;
	UnlockParams( );
	assert( fWetDryMix >= REVERB_WETDRYMIX_MIN && fWetDryMix <= REVERB_WETDRYMIX_MAX );
	return fWetDryMix;
}

// GetReflectionsLevel
inline AkReal32 CAkReverbFXParams::GetReflectionsLevel( )
{
	LockParams( );
	AkReal32 fReflectionsLevel = m_Params.fReflectionsLevel;
	UnlockParams( );
	assert( fReflectionsLevel >= REVERB_LEVEL_MIN && fReflectionsLevel <= REVERB_LEVEL_MAX );
	return fReflectionsLevel;
}

// GetReverbLevel
inline AkReal32 CAkReverbFXParams::GetReverbLevel( )
{
	LockParams( );
	AkReal32 fReverbLevel = m_Params.fReverbLevel;
	UnlockParams( );
	assert( fReverbLevel >= REVERB_LEVEL_MIN && fReverbLevel <= REVERB_LEVEL_MAX );
	return fReverbLevel;
}

// GetReflectionsDelay
inline AkReal32 CAkReverbFXParams::GetReflectionsDelay( )
{
	LockParams( );
	AkReal32 fReflectionsDelay = m_Params.fReflectionsDelay;
	UnlockParams( );
	assert( fReflectionsDelay >= REVERB_REFLECTIONSDELAY_MIN && fReflectionsDelay <= REVERB_REFLECTIONSDELAY_MAX );
	return fReflectionsDelay;
}

// GetReverbDelay
inline AkReal32 CAkReverbFXParams::GetReverbDelay( )
{
	LockParams( );
	AkReal32 fReverbDelay = m_Params.fReverbDelay;
	UnlockParams( );
	assert( fReverbDelay >= REVERB_REVERBDELAY_MIN && fReverbDelay <= REVERB_REVERBDELAY_MAX );
	return fReverbDelay;
}
// GetDecayTime
inline AkReal32 CAkReverbFXParams::GetDecayTime( )
{
	LockParams( );
	AkReal32 fDecayTime = m_Params.fDecayTime;
	UnlockParams( );
	assert( fDecayTime >= REVERB_DECAYTIME_MIN && fDecayTime <= REVERB_DECAYTIME_MAX );
	return fDecayTime;
}

// GetCutoffFreq
inline AkReal32 CAkReverbFXParams::GetCutoffFreq( )
{
	LockParams( );
	AkReal32 fCutoffFreq = m_Params.fCutoffFreq;
	UnlockParams( );
	assert( fCutoffFreq >= REVERB_LPCUTOFFFREQ_MIN && fCutoffFreq <= REVERB_LPCUTOFFFREQ_MAX );
	return fCutoffFreq;
}

// GetReverbDensity
inline AkReal32 CAkReverbFXParams::GetReverbWidth( )
{
	LockParams( );
	AkReal32 fReverbWidth = m_Params.fReverbWidth;
	UnlockParams( );
	assert( fReverbWidth >= REVERB_REVERBWIDTH_MIN && fReverbWidth <= REVERB_REVERBWIDTH_MAX );
	return fReverbWidth;
}

// GetMainLevel
inline AkReal32 CAkReverbFXParams::GetMainLevel( )
{
	LockParams( );
	AkReal32 fMainLevel = m_Params.fMainLevel;
	UnlockParams( );

	// The 0.01f is there to account for some errors that are introduced
	// when converting the RTPC curve's scaling to/from dB/Linear. This should
	// still let us catch "real" errors, when we get really bad values.
	assert( fMainLevel >= REVERB_LEVEL_MIN && fMainLevel <= REVERB_MAINLEVEL_MAX + 0.01f );
	return fMainLevel;
}

// GetProcessLFE
inline bool CAkReverbFXParams::GetProcessLFE( )
{
	LockParams( );
	bool bProcessLFE = m_Params.bProcessLFE;
	UnlockParams( );
	return bProcessLFE;
}

#endif // _AK_REVERBFXPARAMS_H_
