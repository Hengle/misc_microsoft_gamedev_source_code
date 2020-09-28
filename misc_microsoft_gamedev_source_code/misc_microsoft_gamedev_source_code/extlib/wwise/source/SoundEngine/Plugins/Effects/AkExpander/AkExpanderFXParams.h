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
// AkExpanderFXParams.h
//
// Shared parameter implementation for expander FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_EXPANDERPARAMS_H_
#define _AK_EXPANDERPARAMS_H_

#include <AK/Plugin/AkExpanderFXFactory.h>

// Structure of ExpanderFX parameters
struct AkExpanderFXParams
{
	AkReal32		fThreshold;
	AkReal32		fRatio;
	AkReal32		fAttack;
	AkReal32		fRelease;
	AkReal32		fOutputLevel;
	bool			bProcessLFE;
	bool			bChannelLink;
} AK_ALIGNED_16;

#ifndef __SPU__

#include <assert.h>
// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_EXPANDERFXPARAM_THRESHOLD_ID		= 0;	
static const AkPluginParamID AK_EXPANDERFXPARAM_RATIO_ID			= 1;
static const AkPluginParamID AK_EXPANDERFXPARAM_ATTACK_ID			= 2;	
static const AkPluginParamID AK_EXPANDERFXPARAM_RELEASE_ID			= 3;
static const AkPluginParamID AK_EXPANDERFXPARAM_GAIN_ID				= 4;
static const AkPluginParamID AK_EXPANDERFXPARAM_PROCESSLFE_ID		= 5;	
static const AkPluginParamID AK_EXPANDERFXPARAM_CHANNELLINK_ID		= 6;	

// Valid parameter ranges
#define AK_EXPANDER_THRESHOLD_MIN		(-96.3f)
#define AK_EXPANDER_THRESHOLD_MAX		(0.f)
#define AK_EXPANDER_RATIO_MIN			(1.f)
#define AK_EXPANDER_RATIO_MAX			(50.f)
#define AK_EXPANDER_ATTACK_MIN			(0.001f)
#define AK_EXPANDER_ATTACK_MAX			(0.5f)
#define AK_EXPANDER_RELEASE_MIN			(0.001f)
#define AK_EXPANDER_RELEASE_MAX			(0.5f)
#define AK_EXPANDER_GAIN_MIN			(0.056234132f) // (-25 dB to be on the safe side with FP arithmetic)
#define AK_EXPANDER_GAIN_MAX			(17.7827941f)  // (+25 dB to be on the safe side with FP arithmetic)

// Default values in case a bad parameter block is provided
#define AK_EXPANDER_THRESHOLD_DEF		(-30.f)
#define AK_EXPANDER_RATIO_DEF			(4.f)
#define AK_EXPANDER_ATTACK_DEF			(0.1f)
#define AK_EXPANDER_RELEASE_DEF			(0.01f)
#define AK_EXPANDER_GAIN_DEF			(1.f)
#define AK_EXPANDER_PROCESSLFE_DEF		(true)
#define AK_EXPANDER_CHANNELLINK_DEF		(true)

//-----------------------------------------------------------------------------
// Name: class CAkExpanderFXParams
// Desc: Shared dynamics processing FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkExpanderFXParams : public AK::IAkPluginParam
{
public:

    AK_USE_PLUGIN_ALLOCATOR();

	friend class CAkExpanderFX;
    
    // Constructor/destructor.
    CAkExpanderFXParams( );
    ~CAkExpanderFXParams( );
	CAkExpanderFXParams( const CAkExpanderFXParams & in_rCopy );

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

	// Get all parameters at once
	inline void GetParams( AkExpanderFXParams * out_pParams )
	{
		LockParams( );
		*out_pParams = m_Params;
		UnlockParams( );
		// Parameter range check
		assert(	(out_pParams->fThreshold >= AK_EXPANDER_THRESHOLD_MIN) && 
				(out_pParams->fThreshold <= AK_EXPANDER_THRESHOLD_MAX));
		assert(	(out_pParams->fRatio >= AK_EXPANDER_RATIO_MIN) && 
				(out_pParams->fRatio <= AK_EXPANDER_RATIO_MAX));
		assert(	(out_pParams->fAttack >= AK_EXPANDER_ATTACK_MIN) && 
				(out_pParams->fAttack <= AK_EXPANDER_ATTACK_MAX));
		assert(	(out_pParams->fRelease >= AK_EXPANDER_RELEASE_MIN) && 
				(out_pParams->fRelease <= AK_EXPANDER_RELEASE_MAX));
		assert(	(out_pParams->fOutputLevel >= AK_EXPANDER_GAIN_MIN) && 
				(out_pParams->fOutputLevel <= AK_EXPANDER_GAIN_MAX));
	}

	// Get all RTPC parameters at once
	inline void GetRTPCParams( AkExpanderFXParams * out_pParams )
	{
		LockParams( );

		out_pParams->fOutputLevel = m_Params.fOutputLevel;
		out_pParams->fRatio = m_Params.fRatio;
		out_pParams->fAttack = m_Params.fAttack;
		out_pParams->fRelease = m_Params.fRelease;
		out_pParams->fThreshold = m_Params.fThreshold;

		UnlockParams( );
		// Parameter range check
		assert(	(out_pParams->fThreshold >= AK_EXPANDER_THRESHOLD_MIN) && 
				(out_pParams->fThreshold <= AK_EXPANDER_THRESHOLD_MAX));
		assert(	(out_pParams->fRatio >= AK_EXPANDER_RATIO_MIN) && 
				(out_pParams->fRatio <= AK_EXPANDER_RATIO_MAX));
		assert(	(out_pParams->fRelease >= AK_EXPANDER_RELEASE_MIN) && 
				(out_pParams->fRelease <= AK_EXPANDER_RELEASE_MAX));
		assert(	(out_pParams->fOutputLevel >= AK_EXPANDER_GAIN_MIN) && 
				(out_pParams->fOutputLevel <= AK_EXPANDER_GAIN_MAX));
	}
private:

	AkExpanderFXParams	m_Params;						// Parameter structure.
};

#endif // #ifndef __SPU__

#endif // _AK_EXPANDERPARAMS_H_
