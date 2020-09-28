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
// AkCompressorFXParams.cpp
//
// Shared parameter implementation for compressor FX.
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkCompressorFXParams.h"
#include <math.h>

// Creation function
AK::IAkPluginParam * CreateCompressorFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkCompressorFXParams( ) );
}

// Constructor/destructor.
CAkCompressorFXParams::CAkCompressorFXParams( )
{
}

CAkCompressorFXParams::~CAkCompressorFXParams( )
{
}

// Copy constructor.
CAkCompressorFXParams::CAkCompressorFXParams( const CAkCompressorFXParams & in_rCopy )
{
	m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkCompressorFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	return AK_PLUGIN_NEW( in_pAllocator, CAkCompressorFXParams( *this ) );
}

// Init.
AKRESULT CAkCompressorFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										void *					in_pParamsBlock, 
										AkUInt32				in_ulBlockSize )
{
	if ( in_ulBlockSize == 0)
	{
		LockParams();

		// Init default parameters.
		m_Params.fThreshold = AK_COMPRESSOR_THRESHOLD_DEF;
		m_Params.fRatio = AK_COMPRESSOR_RATIO_DEF;
		m_Params.fAttack = AK_COMPRESSOR_ATTACK_DEF;
		m_Params.fRelease = AK_COMPRESSOR_RELEASE_DEF;
		m_Params.fOutputLevel = AK_COMPRESSOR_GAIN_DEF;
		m_Params.bProcessLFE = AK_COMPRESSOR_PROCESSLFE_DEF;
		m_Params.bChannelLink = AK_COMPRESSOR_CHANNELLINK_DEF;

		UnlockParams( );

		return AK_Success;
	}
	return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

// Term.
AKRESULT CAkCompressorFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	AK_PLUGIN_DELETE( in_pAllocator, this );
	return AK_Success;
}

// Blob set.
AKRESULT CAkCompressorFXParams::SetParamsBlock(	void * in_pParamsBlock, 
												AkUInt32 in_ulBlockSize
												)
{  
	LockParams();

	// Read data in the order it was put in the bank
	AkUInt8 * pucParamPtr = reinterpret_cast<AkUInt8 *>( in_pParamsBlock );
	m_Params.fThreshold = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fThreshold );
	m_Params.fRatio = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fRatio );
	m_Params.fAttack = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fAttack );
	m_Params.fRelease = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fRelease );
	m_Params.fOutputLevel  = powf( 10.f, AK::ReadBankReal32( (AkReal32 *) pucParamPtr ) * 0.05f );
	pucParamPtr += sizeof( m_Params.fOutputLevel );
	m_Params.bProcessLFE = *reinterpret_cast<bool *>( pucParamPtr );
	pucParamPtr += sizeof( m_Params.bProcessLFE );
	m_Params.bChannelLink = *reinterpret_cast<bool *>( pucParamPtr );
	pucParamPtr += sizeof( m_Params.bChannelLink );

	assert( (pucParamPtr - reinterpret_cast<AkUInt8 *>( in_pParamsBlock ) ) == in_ulBlockSize );

	UnlockParams();

	return AK_Success;
}

// Update one parameter.
AKRESULT CAkCompressorFXParams::SetParam(	AkPluginParamID in_ParamID,
											void * in_pValue, 
											AkUInt32 in_ulParamSize )
{
	assert( in_pValue != NULL );
	if ( in_pValue == NULL )
	{
		return AK_InvalidParameter;
	}
	AKRESULT eResult = AK_Success;

	LockParams();

	switch ( in_ParamID )
	{
	case AK_COMPRESSORFXPARAM_THRESHOLD_ID:
		m_Params.fThreshold = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_COMPRESSORFXPARAM_RATIO_ID:
		m_Params.fRatio = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_COMPRESSORFXPARAM_ATTACK_ID:
		m_Params.fAttack = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_COMPRESSORFXPARAM_RELEASE_ID:
		m_Params.fRelease = *reinterpret_cast<AkReal32*>(in_pValue);
		break;
	case AK_COMPRESSORFXPARAM_GAIN_ID:
		{
			AkReal32 fDbVal = *reinterpret_cast<AkReal32*>(in_pValue);
			m_Params.fOutputLevel  = powf( 10.f, fDbVal * 0.05f );
		}
		break;
	case AK_COMPRESSORFXPARAM_PROCESSLFE_ID:
		m_Params.bProcessLFE = *reinterpret_cast<bool*>(in_pValue);
		break;
	case AK_COMPRESSORFXPARAM_CHANNELLINK_ID:
		m_Params.bChannelLink = *reinterpret_cast<bool*>(in_pValue);
		break;
	default:
		assert(!"Invalid parameter.");
		eResult = AK_InvalidParameter;
	}

	UnlockParams( );

	return eResult;
}