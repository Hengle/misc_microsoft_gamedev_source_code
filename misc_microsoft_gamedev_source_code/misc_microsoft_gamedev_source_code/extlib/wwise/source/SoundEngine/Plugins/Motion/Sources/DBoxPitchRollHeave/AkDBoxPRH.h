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
// AkPitchRollHeave.h
//
// Silence source, pretty straight forward.
// Note: Target output format is currently determined by the source itself.
// Out format currently used is : 48 kHz, Float, Mono
//
// Copyright (c) 2006-2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_PRH_H_
#define _AKFXSRC_PRH_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Plugin/AkDBoxPitchRollHeaveFactory.h>

#include "..\..\..\..\AkAudioLib\Common\AkConversionTable.h"

// Parameters IDs. To be used by game for RTPC.
//const AkPluginParamID AK_SRCSILENCE_FXPARAM_DUR_ID			= 0;
//...

const AkPluginParamID AK_Pitch_Param		= 0;
const AkPluginParamID AK_Roll_Param			= 1;
const AkPluginParamID AK_Heave_Param		= 2;
const AkPluginParamID AK_Duration_Param		= 3;
const AkPluginParamID AK_FrontLeft_Param	= 4;
const AkPluginParamID AK_FrontRight_Param	= 5;
const AkPluginParamID AK_BackLeft_Param		= 6;
const AkPluginParamID AK_BackRight_Param	= 7;
const AkPluginParamID AK_Simulate_Param		= 50;
const AkPluginParamID AK_Curves_Param		= 51;

enum {CurvePitch = 0, CurveRoll, CurveHeave};
#define AK_NUM_CURVES 3

class AkCurve : public CAkConversionTable<AkRTPCGraphPoint, AkReal32>
{
public:
	AK_USE_PLUGIN_ALLOCATOR();
};

// Parameters structure for this effect.
// The effect implementor has the task of defining its parameter's structure.
struct AkPitchRollHeaveParams
{
	//These parameters are read as a block from the bank
	AkReal32	m_fDuration;
	AkCurve		m_Curves[AK_NUM_CURVES];

	//Not part of banks.
	AkReal32	m_fPitch;
	AkReal32	m_fRoll;
	AkReal32	m_fHeave;
	AkReal32	m_fFrontLeft;
	AkReal32	m_fFrontRight;
	AkReal32	m_fBackLeft;
	AkReal32	m_fBackRight;

#ifndef AK_OPTIMIZED
	bool		m_bSimulate;	
#endif
};

//-----------------------------------------------------------------------------
// Name: class CAkPitchRollHeaveParams
// Desc: Implementation the silence source shared parameters.
//-----------------------------------------------------------------------------
class CAkPitchRollHeaveParams : public AK::IAkPluginParam
{
public:

	AK_USE_PLUGIN_ALLOCATOR();

    // Plugin mechanism. Implement Create function and register its address to the FX manager.
	static AK::IAkPluginParam * CreateEffectParam( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor.
    CAkPitchRollHeaveParams(AK::IAkPluginMemAlloc * in_pAllocator);
    CAkPitchRollHeaveParams( const CAkPitchRollHeaveParams &Copy );
    ~CAkPitchRollHeaveParams();

    // Create duplicate.
	virtual AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
	virtual AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						   
                           void *				in_pvParamsBlock, 
                           AkUInt32				in_ulBlockSize 
                         );
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    virtual AKRESULT SetParamsBlock( void * in_pvParamsBlock, 
                                     AkUInt32 in_ulBlockSize
                                   );

    // Update one parameter.
	virtual AKRESULT SetParam( AkPluginParamID in_ParamID,
                               void * in_pvValue, 
                               AkUInt32 in_ulParamSize
                             );

	inline AkPitchRollHeaveParams& GetParams() {return m_Params;}
private:
	AKRESULT ReadCurve(AkUInt16 in_iIndex, void* &io_pData);
	AKRESULT ReadAllCurves(void* &io_pData);
	void	 SetSimulate(bool in_bEnabled);

	// Parameter structure.
    AkPitchRollHeaveParams m_Params;
	AK::IAkPluginMemAlloc * m_pAllocator;
};

//-----------------------------------------------------------------------------
// Name: class CAkPitchRollHeave
// Desc: Implementation of a generator for rumble pads
//-----------------------------------------------------------------------------
class CAkPitchRollHeave : public AK::IAkSourcePlugin
{
public:

	AK_USE_PLUGIN_ALLOCATOR();

	// Plugin mechanism. Implement Create function and register its address to the FX manager.
	static IAkPlugin* CreateEffect( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor
    CAkPitchRollHeave();
    virtual ~CAkPitchRollHeave(); 

    // Initialize
	virtual AKRESULT Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
							AK::IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
							AK::IAkPluginParam *			in_pParams,			// Effect parameters.
							AkAudioFormat &					io_rFormat			// Supported audio output format.
                         );

    // Terminate
    // NOTE: The effect must DELETE ITSELF herein.
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset or seek to start (looping).
	virtual AKRESULT Reset( );

    // Info query:
    // Effect type (source, monadic, mixer, ...)
    // Buffer type: in-place, input(?), output (source), io.
    virtual AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute( AkAudioBuffer * io_pBufferOut );	// Output buffer interface.

	virtual AkTimeMs GetDuration() const;
	virtual AKRESULT StopLooping();

private:

	AkReal32 ComputeDuration();

    // Internal state variables.
    AkUInt32			m_uSampleRate;			
	AkUInt32			m_uBytesPerSample;
	AkUInt32			m_uDuration;			//Duration of the source (without loops)
	AkUInt32			m_uSamplesProduced;		//Samples produced up to now (always less than m_uDuration)
	AkUInt32			m_uLoops;				//Loops to complete
	AkReal32			m_fOneSample;
	AkReal32			m_fTime;

	AkReal32	m_fOldPitch;
	AkReal32	m_fOldRoll;
	AkReal32	m_fOldHeave;
	AkReal32	m_fOldFrontLeft;
	AkReal32	m_fOldFrontRight;
	AkReal32	m_fOldBackLeft;
	AkReal32	m_fOldBackRight;

#ifndef AK_OPTIMIZE
	bool		m_bOldSimulate;
#endif

    // Shared parameters structure
    CAkPitchRollHeaveParams * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;
};

#endif  //_AKFXSRC_PRH_H_
