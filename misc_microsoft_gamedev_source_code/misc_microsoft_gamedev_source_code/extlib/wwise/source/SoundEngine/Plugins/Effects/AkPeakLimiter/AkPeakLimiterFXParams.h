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
// AkPeakLimiterFXParams.h
//
// Shared parameter implementation for peak limiter FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PEAKLIMITERPARAMS_H_
#define _AK_PEAKLIMITERPARAMS_H_

#include <AK/Plugin/AkPeakLimiterFXFactory.h>

// Structure of PeakLimiterFX parameters
struct AkPeakLimiterFXParams
{
	AkReal32		fThreshold;
	AkReal32		fRatio;
	AkReal32		fLookAhead;
	AkReal32		fRelease;
	AkReal32		fOutputLevel;
	bool			bProcessLFE;
	bool			bChannelLink;
} AK_ALIGNED_16;

#ifndef __SPU__	// The rest need not to be known on SPU side

#include <assert.h>

// Parameters IDs for the Wwise or RTPC.
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_THRESHOLD_ID		= 0;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_RATIO_ID			= 1;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_LOOKAHEAD_ID		= 2;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_RELEASE_ID		= 3;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_GAIN_ID			= 4;
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_PROCESSLFE_ID	= 5;	
static const AkPluginParamID AK_PEAKLIMITERFXPARAM_CHANNELLINK_ID	= 6;	

// Valid parameter ranges
#define AK_PEAKLIMITER_THRESHOLD_MIN	(-96.3f)
#define AK_PEAKLIMITER_THRESHOLD_MAX	(0.f)
#define AK_PEAKLIMITER_RATIO_MIN		(1.f)
#define AK_PEAKLIMITER_RATIO_MAX		(50.f)
#define AK_PEAKLIMITER_LOOKAHEAD_MIN	(0.001f)
#define AK_PEAKLIMITER_LOOKAHEAD_MAX	(0.020f)
#define AK_PEAKLIMITER_RELEASE_MIN		(0.001f)
#define AK_PEAKLIMITER_RELEASE_MAX		(0.5f)
#define AK_PEAKLIMITER_GAIN_MIN			(0.056234132f) // (-25 dB to be on the safe side with FP arithmetic)
#define AK_PEAKLIMITER_GAIN_MAX			(17.7827941f)  // (+25 dB to be on the safe side with FP arithmetic)

// Default values in case a bad parameter block is provided
#define AK_PEAKLIMITER_THRESHOLD_DEF	(-12.f)
#define AK_PEAKLIMITER_RATIO_DEF		(10.f)
#define AK_PEAKLIMITER_LOOKAHEAD_DEF	(0.01f)
#define AK_PEAKLIMITER_RELEASE_DEF		(0.2f)
#define AK_PEAKLIMITER_GAIN_DEF			(1.f) // linear value
#define AK_PEAKLIMITER_PROCESSLFE_DEF	(true)
#define AK_PEAKLIMITER_CHANNELLINK_DEF	(true)

//-----------------------------------------------------------------------------
// Name: class CAkPeakLimiterFXParams
// Desc: Shared dynamics processing FX parameters implementation.
//-----------------------------------------------------------------------------
class CAkPeakLimiterFXParams : public AK::IAkPluginParam
{
public:

    AK_USE_PLUGIN_ALLOCATOR();

	friend class CAkPeakLimiterFX;
    
    // Constructor/destructor.
    CAkPeakLimiterFXParams( );
    ~CAkPeakLimiterFXParams( );
	CAkPeakLimiterFXParams( const CAkPeakLimiterFXParams & in_rCopy );

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
	inline void GetParams( AkPeakLimiterFXParams * out_pParams )
	{
		LockParams( );

		*out_pParams = m_Params;

		UnlockParams( );
		// Parameter range check
		assert(	(out_pParams->fThreshold >= AK_PEAKLIMITER_THRESHOLD_MIN) && 
				(out_pParams->fThreshold <= AK_PEAKLIMITER_THRESHOLD_MAX));
		assert(	(out_pParams->fRatio >= AK_PEAKLIMITER_RATIO_MIN) && 
				(out_pParams->fRatio <= AK_PEAKLIMITER_RATIO_MAX));
		assert(	(out_pParams->fLookAhead >= AK_PEAKLIMITER_LOOKAHEAD_MIN) && 
				(out_pParams->fLookAhead <= AK_PEAKLIMITER_LOOKAHEAD_MAX));
		assert(	(out_pParams->fRelease >= AK_PEAKLIMITER_RELEASE_MIN) && 
				(out_pParams->fRelease <= AK_PEAKLIMITER_RELEASE_MAX));
		assert(	(out_pParams->fOutputLevel >= AK_PEAKLIMITER_GAIN_MIN) && 
				(out_pParams->fOutputLevel <= AK_PEAKLIMITER_GAIN_MAX));
	}

	// Get all RTPC parameters at once
	inline void GetRTPCParams( AkPeakLimiterFXParams * out_pParams )
	{
		LockParams( );

		out_pParams->fOutputLevel = m_Params.fOutputLevel;
		out_pParams->fRatio = m_Params.fRatio;
		out_pParams->fRelease = m_Params.fRelease;
		out_pParams->fThreshold = m_Params.fThreshold;

		UnlockParams( );
		// Parameter range check
		assert(	(out_pParams->fThreshold >= AK_PEAKLIMITER_THRESHOLD_MIN) && 
				(out_pParams->fThreshold <= AK_PEAKLIMITER_THRESHOLD_MAX));
		assert(	(out_pParams->fRatio >= AK_PEAKLIMITER_RATIO_MIN) && 
				(out_pParams->fRatio <= AK_PEAKLIMITER_RATIO_MAX));
		assert(	(out_pParams->fRelease >= AK_PEAKLIMITER_RELEASE_MIN) && 
				(out_pParams->fRelease <= AK_PEAKLIMITER_RELEASE_MAX));
		assert(	(out_pParams->fOutputLevel >= AK_PEAKLIMITER_GAIN_MIN) && 
				(out_pParams->fOutputLevel <= AK_PEAKLIMITER_GAIN_MAX));
	}

private:

    AkPeakLimiterFXParams	m_Params;						// Parameter structure.
};

#endif // #ifndef __SPU__

#endif // _AK_PEAKLIMITERPARAMS_H_
