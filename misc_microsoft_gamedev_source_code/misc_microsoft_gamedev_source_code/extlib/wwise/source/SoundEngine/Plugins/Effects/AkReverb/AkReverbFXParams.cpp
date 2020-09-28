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
// AkReverbFXParams.cpp
//
// Reverb FX parameters implementation
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkReverbFXParams.h"

// Creation function
AK::IAkPluginParam * CreateReverbFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkReverbFXParams( ) );
}

// Constructor/destructor.
CAkReverbFXParams::CAkReverbFXParams( )
{
}

CAkReverbFXParams::~CAkReverbFXParams( )
{
}

// Copy constructor.
CAkReverbFXParams::CAkReverbFXParams( const CAkReverbFXParams & in_rCopy )
{
    m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkReverbFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkReverbFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkReverbFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
									void *					in_pParamsBlock, 
									AkUInt32				in_ulBlockSize 
								 )
{
	assert( in_pAllocator != NULL );
	if ( in_ulBlockSize == 0)
	{
		LockParams( );

		// Init default parameters.
		m_Params.fWetDryMix			= REVERB_WETDRYMIX_DEF;
		m_Params.fReflectionsLevel	= REVERB_REFLECTIONSLEVEL_DEF;
		m_Params.fReverbLevel		= REVERB_REVERBLEVEL_DEF;
		m_Params.fReflectionsDelay	= REVERB_REFLECTIONSDELAY_DEF;
		m_Params.fReverbDelay		= REVERB_REVERBDELAY_DEF;
		m_Params.fDecayTime			= REVERB_DECAYTIME_DEF;
		m_Params.fCutoffFreq		= REVERB_LPCUTOFFFREQ_DEF; 
		m_Params.fReverbWidth		= REVERB_REVERBWIDTH_DEF; 
		m_Params.fMainLevel			= REVERB_MAINLEVEL_DEF;
		m_Params.bProcessLFE		= REVERB_PROCESSLFE_DEF;

		UnlockParams( );
		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkReverbFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkReverbFXParams::SetParamsBlock( void * in_pParamsBlock, 
											AkUInt32 in_ulBlockSize
										   )
{ 
	LockParams( );

	AkReal32 fTemp;

	// Read data in the order it was put in the bank
	AkUInt8 * pucParamPtr = reinterpret_cast<AkUInt8 *>( in_pParamsBlock );
	m_Params.fWetDryMix = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fWetDryMix );
	fTemp = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	m_Params.fReflectionsLevel = AK::dBToLin(fTemp); // Make it a linear value
	pucParamPtr += sizeof( m_Params.fReflectionsLevel );
	fTemp = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	m_Params.fReverbLevel = AK::dBToLin( fTemp ); // Make it a linear value
	pucParamPtr += sizeof( m_Params.fReverbLevel );
	m_Params.fReflectionsDelay = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fReflectionsDelay );
	m_Params.fReverbDelay = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fReverbDelay );
	m_Params.fDecayTime = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fDecayTime );
	m_Params.fCutoffFreq = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fCutoffFreq );
	m_Params.fReverbWidth = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fReverbWidth );
	// this one goes up to +12 dB so AK::dBToLin cannot be used
	fTemp = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	m_Params.fMainLevel = powf( 10.0f, ( fTemp * 0.05f ) ); // Make it a linear value	
	pucParamPtr += sizeof( m_Params.fMainLevel );
	m_Params.bProcessLFE = *reinterpret_cast<bool *>( pucParamPtr );
	pucParamPtr += sizeof( m_Params.bProcessLFE );
	assert( (pucParamPtr - reinterpret_cast<AkUInt8 *>( in_pParamsBlock ) ) == in_ulBlockSize );

	UnlockParams( );

	return AK_Success;
}

// Update one parameter.
AKRESULT CAkReverbFXParams::SetParam(	AkPluginParamID in_ParamID,
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
	case AK_REVERBFXPARAM_WETDRYMIX_ID:
		m_Params.fWetDryMix = *reinterpret_cast<AkReal32*>(in_pValue);;
		break;
	case AK_REVERBFXPARAM_REFLECTIONSLEVEL_ID:
		{
			AkReal32 fReflectionsLevel = *reinterpret_cast<AkReal32*>(in_pValue);
			m_Params.fReflectionsLevel = powf( 10.f, ( fReflectionsLevel / 20.f ) ); // Make it a linear value
		}
		break;
	case AK_REVERBFXPARAM_REVERBLEVEL_ID:
		{
			AkReal32 fReverbLevel = *reinterpret_cast<AkReal32*>(in_pValue);
			m_Params.fReverbLevel = AK::dBToLin( fReverbLevel ); // Make it a linear value
		}
		break;
	case AK_REVERBFXPARAM_REFLECTIONSDELAY_ID:
		m_Params.fReflectionsDelay = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_REVERBFXPARAM_REVERBDELAY_ID:
		m_Params.fReverbDelay = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_REVERBFXPARAM_DECAYTIME_ID:
		m_Params.fDecayTime = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_REVERBFXPARAM_LPCUTOFFFREQ_ID:
		m_Params.fCutoffFreq = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_REVERBFXPARAM_REVERBWIDTH_ID:
		m_Params.fReverbWidth = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_REVERBFXPARAM_MAINLEVEL_ID:
		{
			AkReal32 fMainLevel = *reinterpret_cast<AkReal32*>(in_pValue);
			m_Params.fMainLevel = powf( 10.f, ( fMainLevel / 20.f ) ); // Make it a linear value	
		}
		break;
	case AK_REVERBFXPARAM_PROCESSLFE_ID:
		m_Params.bProcessLFE = *reinterpret_cast<bool*>(in_pValue);
		break;

	default:
		assert(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	UnlockParams( );
	return eResult;

}