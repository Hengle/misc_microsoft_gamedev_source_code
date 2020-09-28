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
// AkSimpleVerbFXParams.cpp
//
// Reverb FX parameters implementation
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkSimpleVerbFXParams.h"

// Creation function
AK::IAkPluginParam * CreateReverbLiteFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    return AK_PLUGIN_NEW( in_pAllocator, CAkSimpleVerbFXParams( ) );
}

// Constructor/destructor.
CAkSimpleVerbFXParams::CAkSimpleVerbFXParams( )
{
}

CAkSimpleVerbFXParams::~CAkSimpleVerbFXParams( )
{
}

// Copy constructor.
CAkSimpleVerbFXParams::CAkSimpleVerbFXParams( const CAkSimpleVerbFXParams & in_rCopy )
{
    m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkSimpleVerbFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    return AK_PLUGIN_NEW( in_pAllocator, CAkSimpleVerbFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkSimpleVerbFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										void *					in_pParamsBlock, 
										AkUInt32				in_ulBlockSize 
                                     )
{
	assert( in_pAllocator != NULL );
    if ( in_ulBlockSize == 0)
    {
        LockParams( );
        // Init default parameters.
		m_Params.fWetDryMix		= SIMPLEVERBFXPARAM_WETDRYMIX_DEF;
		m_Params.fReverbTime	= SIMPLEVERBFXPARAM_REVERBTIME_DEF;
		m_Params.fCutoffFreq	= SIMPLEVERBFXPARAM_LPCUTOFFFREQ_DEF; 
		m_Params.fMainLevel		= SIMPLEVERBFXPARAM_MAINLEVEL_DEF;
		m_Params.bProcessLFE	= SIMPLEVERBFXPARAM_PROCESSLFE_DEF;
        UnlockParams( );
        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkSimpleVerbFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Blob set.
AKRESULT CAkSimpleVerbFXParams::SetParamsBlock(	void * in_pParamsBlock, 
                                                AkUInt32 in_ulBlockSize
                                               )
{
    if ( NULL == in_pParamsBlock )
    {
    	assert( !"Invalid parameter block." );
        return AK_InvalidParameter;
    }

	LockParams( );

	// Read data in the order it was put in the bank
	AkUInt8 * pucParamPtr = reinterpret_cast<AkUInt8 *>( in_pParamsBlock );
	m_Params.fWetDryMix = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fWetDryMix );
	m_Params.fReverbTime = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fReverbTime );
	m_Params.fCutoffFreq = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fCutoffFreq );
	AkReal32 fTemp = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	m_Params.fMainLevel = AK::dBToLin(fTemp);
	pucParamPtr += sizeof( m_Params.fMainLevel );
	m_Params.bProcessLFE = *reinterpret_cast<bool *>( pucParamPtr );
	pucParamPtr += sizeof( m_Params.bProcessLFE );
	assert( (pucParamPtr - reinterpret_cast<AkUInt8 *>( in_pParamsBlock ) ) == in_ulBlockSize );

	UnlockParams( );

    return AK_Success;
}

// Update one parameter.
AKRESULT CAkSimpleVerbFXParams::SetParam(	AkPluginParamID in_ParamID,
											void * in_pValue, 
											AkUInt32 in_ulParamSize
                                         )
{
	assert( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}

	AKRESULT eResult = AK_Success;

	LockParams( );

	switch ( in_ParamID )
	{
	case AK_SIMPLEVERBFXPARAM_WETDRYMIX_ID:	
		m_Params.fWetDryMix = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_SIMPLEVERBFXPARAM_REVERBTIME_ID:
		m_Params.fReverbTime = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_SIMPLEVERBFXPARAM_LPCUTOFFFREQ_ID:
		m_Params.fCutoffFreq = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_SIMPLEVERBFXPARAM_MAINLEVEL_ID:
	{
		AkReal32 fTemp = *reinterpret_cast<AkReal32*>(in_pValue);
		m_Params.fMainLevel = AK::dBToLin(fTemp); // Make it a linear value
	}
		break;
	case AK_SIMPLEVERBFXPARAM_PROCESSLFE_ID:
		m_Params.bProcessLFE = *reinterpret_cast<bool*>(in_pValue);
		break;
	default:
		assert(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	UnlockParams( );
	return eResult;

}