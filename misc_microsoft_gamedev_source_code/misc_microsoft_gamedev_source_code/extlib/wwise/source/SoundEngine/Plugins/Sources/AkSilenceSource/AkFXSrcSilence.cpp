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
// AkFXSrcSilence.cpp
//
// Implements a silent source.
// Note: Target output format is currently determined by the source itself.
// Out format currently used is : 48 kHz, 16 bits, Mono
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFXSrcSilence.h"

#ifdef RVL_OS
#include <string.h>
#include <stdlib.h>
#endif

using namespace AK;

// Plugin mechanism. Implement Create function and register its address to the FX manager.
IAkPluginParam * CreateSilenceSourceParams( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcSilenceParams( ) );
}

// Constructor.
CAkFxSrcSilenceParams::CAkFxSrcSilenceParams()
{
}

// Copy constructor.
CAkFxSrcSilenceParams::CAkFxSrcSilenceParams( const CAkFxSrcSilenceParams &Copy )
{
    m_Params = Copy.m_Params;
}

// Destructor.
CAkFxSrcSilenceParams::~CAkFxSrcSilenceParams()
{
}

// Create shared parameters duplicate.
IAkPluginParam * CAkFxSrcSilenceParams::Clone( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFxSrcSilenceParams( *this ) );
}

// Shared parameters initialization.
AKRESULT CAkFxSrcSilenceParams::Init( IAkPluginMemAlloc *	in_pAllocator,									  
	                                  void *			in_pvParamsBlock, 
	                                  AkUInt32			in_ulBlockSize 
                                 )
{
    if ( in_ulBlockSize == 0)
    {
        LockParams( );

        // Init with default if we got invalid parameter block.
        m_Params.fDuration = 1.f;				// Duration (secs).     
        m_Params.fRandomizedLengthMinus = 0.f;  // Maximum randomness to subtract to duration (secs)
		m_Params.fRandomizedLengthPlus = 0.f;   // Maximum randomness to add to duration (secs) 

        UnlockParams( );
        return AK_Success;
    }
	 
    return SetParamsBlock( in_pvParamsBlock, in_ulBlockSize );
}

// Shared parameters termination.
AKRESULT CAkFxSrcSilenceParams::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Set all shared parameters at once.
AKRESULT CAkFxSrcSilenceParams::SetParamsBlock( void * in_pvParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                              )
{

    assert( NULL != in_pvParamsBlock && sizeof(AkFXSrcSilenceParams) == in_ulBlockSize );
    if ( NULL == in_pvParamsBlock || sizeof(AkFXSrcSilenceParams) != in_ulBlockSize )
    {
        return AK_InvalidParameter;
    }

	LockParams( );

	memcpy( &m_Params, in_pvParamsBlock, sizeof( AkFXSrcSilenceParams ) );
    
    UnlockParams( );

    return AK_Success;
}

// Update one parameter.
AKRESULT CAkFxSrcSilenceParams::SetParam( AkPluginParamID in_ParamID,
                                          void * in_pvValue, 
                                          AkUInt32 in_ulParamSize
                                        )
{
    if ( in_pvValue == NULL )
	{
		return AK_InvalidParameter;
	}

	// Pointer should be aligned on 4 bytes
#if defined(WIN32) || defined(XBOX360)
	assert(((__w64 int)in_pvValue & 3) == 0);
#else
	assert(((int)in_pvValue & 3) == 0);
#endif

	LockParams( );

    // Set parameter value.
    switch ( in_ParamID )
    {
	case AK_SRCSILENCE_FXPARAM_DUR_ID:
		m_Params.fDuration = *reinterpret_cast<AkReal32*>(in_pvValue);
		break;
	case AK_SRCSILENCE_FXPARAM_RANDMINUS_ID:
		m_Params.fRandomizedLengthMinus = *reinterpret_cast<AkReal32*>(in_pvValue);
		break;
	case AK_SRCSILENCE_FXPARAM_RANDPLUS_ID:
		m_Params.fRandomizedLengthPlus = *reinterpret_cast<AkReal32*>(in_pvValue);
		break;
	default:
		UnlockParams( );
		return AK_InvalidParameter;
    }

    UnlockParams( );

    return AK_Success;
}
 
//-----------------------------------------------------------------------------
// Name: CreateEffect
// Desc: Plugin mechanism. Dynamic create function whose address must be 
//       registered to the FX manager.
//-----------------------------------------------------------------------------
IAkPlugin* CreateSilenceSource( IAkPluginMemAlloc * in_pAllocator )
{
    return AK_PLUGIN_NEW( in_pAllocator, CAkFXSrcSilence( ) );
}

//-----------------------------------------------------------------------------
// Name: CAkFXSrcSilence
// Desc: Constructor.
//-----------------------------------------------------------------------------
CAkFXSrcSilence::CAkFXSrcSilence()
{
	// Initialize members.
	m_uSampleRate = 0;
	m_uBytesPerSample = 0;
	m_sNumLoops = 1;
	m_sCurLoopCount = 0;
	m_fDurationModifier = 0.f;
	m_fInitDuration = 0.f;
	m_pSourceFXContext = NULL;
	m_pSharedParams = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CAkFXSrcSilence
// Desc: Destructor.
//-----------------------------------------------------------------------------
CAkFXSrcSilence::~CAkFXSrcSilence()
{

}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Init( IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
								IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
								IAkPluginParam *			in_pParams,			// Effect parameters.
								AkAudioFormat &				io_rFormat			// Supported audio output format.
								)
{
	// Keep source FX context (looping etc.)
	m_pSourceFXContext = in_pSourceFXContext;

	// Save audio format internally
	m_uSampleRate = io_rFormat.uSampleRate;
	m_uBytesPerSample = io_rFormat.GetBitsPerSample() / 8;

	// Looping info.
	m_sNumLoops = m_pSourceFXContext->GetNumLoops( );
	assert( m_sNumLoops >= 0 );

    // Set parameters access.
    assert( NULL != in_pParams );
    m_pSharedParams = reinterpret_cast<CAkFxSrcSilenceParams*>(in_pParams);

	// Compute modifier (offset value)
	AkReal32 fRandomPlusMax = m_pSharedParams->GetRandomPlus( );
	AkReal32 fRandomMinusMax = m_pSharedParams->GetRandomMinus( );
	assert(fRandomPlusMax >= 0.f);
	assert(fRandomMinusMax <= 0.f);
	m_fDurationModifier = RandRange(fRandomMinusMax,fRandomPlusMax);

	m_fInitDuration = m_pSharedParams->GetDuration( ) + m_fDurationModifier;
	if ( m_fInitDuration < SILENCE_DURATION_MIN )
	{
		m_fInitDuration = SILENCE_DURATION_MIN;
	}

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term.
// Desc: Term. The effect must destroy itself herein
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Term( IAkPluginMemAlloc * in_pAllocator )
{
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset or seek to start (looping).
//-----------------------------------------------------------------------------
AKRESULT CAkFXSrcSilence::Reset( void )
{
    m_ulOutByteCount = 0;
	m_sCurLoopCount = 0;
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetEffectType
// Desc: Effect type query.
//-----------------------------------------------------------------------------
// Info query:
// Effect type (source, monadic, mixer, ...)
// Buffer type: in-place, input(?), output (source), io.
AKRESULT CAkFXSrcSilence::GetPluginInfo( AkPluginInfo & out_rPluginInfo )
{
    out_rPluginInfo.eType = AkPluginTypeSource;
	out_rPluginInfo.bIsInPlace = true;
	out_rPluginInfo.bIsAsynchronous = false;
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Execute
// Desc: Effect processing.
//-----------------------------------------------------------------------------
void CAkFXSrcSilence::Execute(	AkAudioBuffer *							io_pBufferOut		// Output buffer interface.
#ifdef AK_PS3
								, AK::MultiCoreServices::DspProcess*&	out_pDspProcess 	// the process that needs to run
#endif
								)
{
	AkUInt32 uBufferSize = io_pBufferOut->MaxFrames() * m_uBytesPerSample;

	AkUInt32 ulProcessSize;	
	io_pBufferOut->eState = AK_DataReady;

	// Check if infinite
	if ( m_sNumLoops == 0 )
	{
		// Silence duration does not matter
		ulProcessSize = uBufferSize;
		// Write silence
		memset(io_pBufferOut->GetChannel(0),0,ulProcessSize);
		// We don't care about the byte counter and loop counts since it is infinite
	}
	// Otherwise is finite
	else
	{
		// Apply random modifier
		AkReal32 fDuration = m_pSharedParams->GetDuration( ) + m_fDurationModifier;
		if ( fDuration < SILENCE_DURATION_MIN )
		{
			fDuration = SILENCE_DURATION_MIN;
		}
		// Multiplying this way ensure proper number of blocks output
		AkUInt32 ulRandomDur = static_cast<AkUInt32>(fDuration * m_uSampleRate) 
			* m_sNumLoops * m_uBytesPerSample;
		// ulRandomDur may have changed and we may have already output to much data
		if ( m_ulOutByteCount >= ulRandomDur )
		{
			// Output no more and stop.
			// Do not updateSizeProcessed with 0
			io_pBufferOut->eState = AK_NoMoreData;
			return;
		}
		else
		{
			// Compute size to be processed
			ulProcessSize = PluginMin( uBufferSize, ulRandomDur - m_ulOutByteCount );

			// Write silence
			memset( io_pBufferOut->GetChannel(0), 0, ulProcessSize );

			// Update production counter
			m_ulOutByteCount += ulProcessSize;

			// Check for the end of this iteration.
			if ( ulProcessSize < uBufferSize )
			{
				// This is the last buffer
				io_pBufferOut->eState = AK_NoMoreData;
			}
		}
	}

	// Notify buffers of updated production
	io_pBufferOut->uValidFrames = (AkUInt16)( ulProcessSize / m_uBytesPerSample );
}


//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkFXSrcSilence::GetDuration( void ) const
{
	return static_cast<AkTimeMs>( m_fInitDuration * 1000.f ) * m_sNumLoops;
}

AKRESULT CAkFXSrcSilence::StopLooping()
{
	// Just make sure we finish within the timing of maximum one more loop.
	m_sNumLoops = 1;
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: RandRange returns a random float value between in_fMin and in_fMax
//-----------------------------------------------------------------------------
AkReal32 CAkFXSrcSilence::RandRange( AkReal32 in_fMin, AkReal32 in_fMax )
{
	// Get an integer in range (0,1.)
	AkReal32 fRandVal = rand() / static_cast<AkReal32>(RAND_MAX);
	return ( fRandVal * (in_fMax - in_fMin) + in_fMin );
}
