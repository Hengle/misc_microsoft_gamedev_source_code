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
// AkSimpleVerbFXParams.h
//
// Shared parameter implementation for reverb FX.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SIMPLEVERBFXPARAMS_H_
#define _AK_SIMPLEVERBFXPARAMS_H_

#include "AkCommonReverbFXParams.h"

#include <AK/Plugin/AkReverbLiteFXFactory.h>
#include <assert.h>
#include <math.h>
#include "AkDbToLin.h"

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_WETDRYMIX_ID			= 0;	// RTPC
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_REVERBTIME_ID			= 1;	// RTPC
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_LPCUTOFFFREQ_ID		= 2;	// RTPC
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_MAINLEVEL_ID			= 3;	// RTPC
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_PROCESSLFE_ID			= 4;
static const AkPluginParamID AK_SIMPLEVERBFXPARAM_NUM					= 5;

// Valid parameter ranges
#define SIMPLEVERBFXPARAM_LEVEL_MIN			0.0f		// linear
#define SIMPLEVERBFXPARAM_LEVEL_MAX			1.0f		// linear
#define SIMPLEVERBFXPARAM_WETDRYMIX_MIN		0.0f		// Percent
#define SIMPLEVERBFXPARAM_WETDRYMIX_DEF		50.0f		// Percent
#define SIMPLEVERBFXPARAM_WETDRYMIX_MAX		100.0f		// Percent
#define SIMPLEVERBFXPARAM_REVERBTIME_MIN	0.2f		// sec
#define SIMPLEVERBFXPARAM_REVERBTIME_DEF	2.0f		// sec
#define SIMPLEVERBFXPARAM_REVERBTIME_MAX	10.0f		// sec
#define SIMPLEVERBFXPARAM_LPCUTOFFFREQ_MIN	20.0f		// Hz
#define SIMPLEVERBFXPARAM_LPCUTOFFFREQ_DEF	7000.0f		// Hz
#define SIMPLEVERBFXPARAM_LPCUTOFFFREQ_MAX	20000.0f	// Hz
#define SIMPLEVERBFXPARAM_MAINLEVEL_DEF		0.25118864315095801110850320677993f		// linear
#define SIMPLEVERBFXPARAM_PROCESSLFE_DEF	(true)

//-----------------------------------------------------------------------------
// Structures.
//-----------------------------------------------------------------------------

// Structure of parameters that remain true for the whole lifespan of the tone generator.
struct AkSimpleVerbFXParams
{
	AkReal32	fWetDryMix;
	AkReal32	fReverbTime;
	AkReal32	fCutoffFreq;
	AkReal32	fMainLevel;
	bool		bProcessLFE;
};

//-----------------------------------------------------------------------------
// Name: class CAkSimpleVerbFXParams
// Desc: Shared reverb FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkSimpleVerbFXParams : public AK::IAkPluginParam
{
public:

    AK_USE_PLUGIN_ALLOCATOR();

	friend class CAkSimpleVerbFX;
    
    // Constructor/destructor.
    CAkSimpleVerbFXParams( );
    ~CAkSimpleVerbFXParams();
	CAkSimpleVerbFXParams( const CAkSimpleVerbFXParams & in_rCopy );

    // Create duplicate.
    IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
    AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						    
                   void *					in_pParamsBlock, 
                   AkUInt32					in_ulBlockSize 
                   );
    AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Blob set.
    AKRESULT SetParamsBlock( void * in_pParamsBlock, 
                             AkUInt32 in_ulBlockSize
                             );

    // Update one parameter.
    AKRESULT SetParam(	AkPluginParamID in_ParamID,
                        void * in_pValue, 
                        AkUInt32 in_ulParamSize
                        );

private:

	void GetParams(AkSimpleVerbFXParams* in_pParams)
	{
		LockParams();
		*in_pParams = m_Params;
		UnlockParams();
	}

	inline AkReal32	GetWetDryMix();
	inline AkReal32	GetReverbTime();
	inline AkReal32	GetCutoffFreq();
	inline AkReal32	GetMainLevel();
	inline bool		GetProcessLFE();

private:

    // Parameter blob.
    AkSimpleVerbFXParams m_Params;
};

// GetWetDryMix
inline AkReal32 CAkSimpleVerbFXParams::GetWetDryMix( )
{
	LockParams( );
	AkReal32 fWetDryMix = m_Params.fWetDryMix;
	UnlockParams( );
	assert((fWetDryMix >= SIMPLEVERBFXPARAM_WETDRYMIX_MIN) && (fWetDryMix <= SIMPLEVERBFXPARAM_WETDRYMIX_MAX));
	return fWetDryMix;
}
// GetReverbTime
inline AkReal32 CAkSimpleVerbFXParams::GetReverbTime( )
{
	LockParams( );
	AkReal32 fReverbTime = m_Params.fReverbTime;
	UnlockParams( );
	assert((fReverbTime >= SIMPLEVERBFXPARAM_REVERBTIME_MIN) && (fReverbTime <= SIMPLEVERBFXPARAM_REVERBTIME_MAX));
	return fReverbTime;
}

// GetCutoffFreq
inline AkReal32 CAkSimpleVerbFXParams::GetCutoffFreq( )
{
	LockParams( );
	AkReal32 fCutoffFreq = m_Params.fCutoffFreq;
	UnlockParams( );
	assert((fCutoffFreq >= SIMPLEVERBFXPARAM_LPCUTOFFFREQ_MIN) && (fCutoffFreq <= SIMPLEVERBFXPARAM_LPCUTOFFFREQ_MAX));
	return fCutoffFreq;
}

// GetMainLevel
inline AkReal32 CAkSimpleVerbFXParams::GetMainLevel( )
{
	LockParams( );
	AkReal32 fMainLevel = m_Params.fMainLevel;
	UnlockParams( );
	assert((fMainLevel >= SIMPLEVERBFXPARAM_LEVEL_MIN) && (fMainLevel <= SIMPLEVERBFXPARAM_LEVEL_MAX));
	return fMainLevel;
}

// GetProcessLFE
inline bool CAkSimpleVerbFXParams::GetProcessLFE( )
{
	LockParams( );
	bool bProcessLFE = m_Params.bProcessLFE;
	UnlockParams( );
	return bProcessLFE;
}

#endif // _AK_SIMPLEVERBFXPARAMS_H_
