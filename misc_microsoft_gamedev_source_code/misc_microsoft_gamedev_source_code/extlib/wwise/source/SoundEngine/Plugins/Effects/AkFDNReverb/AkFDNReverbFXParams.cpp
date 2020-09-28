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
// AkFDNReverbFXParams.cpp
//
// FDN Reverb FX parameters implementation
//
// Copyright 2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkFDNReverbFXParams.h"

// Creation function
AK::IAkPluginParam * CreateMatrixReverbFXParams( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFXParams( ) );
}

// Constructor/destructor.
CAkFDNReverbFXParams::CAkFDNReverbFXParams( )
{
}

CAkFDNReverbFXParams::~CAkFDNReverbFXParams( )
{
}

// Copy constructor.
CAkFDNReverbFXParams::CAkFDNReverbFXParams( const CAkFDNReverbFXParams & in_rCopy )
{
    m_Params = in_rCopy.m_Params;
}

// Create duplicate.
AK::IAkPluginParam * CAkFDNReverbFXParams::Clone( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    return AK_PLUGIN_NEW( in_pAllocator, CAkFDNReverbFXParams( *this ) );
}

// Init/Term.
AKRESULT CAkFDNReverbFXParams::Init(	AK::IAkPluginMemAlloc *	in_pAllocator,									   
										void *					in_pParamsBlock, 
										AkUInt32				in_ulBlockSize 
                                     )
{
	assert( in_pAllocator != NULL );
    if ( in_ulBlockSize == 0)
    {
        LockParams( );
        // Init default parameters.
		m_Params.fReverbTime = AK_FDNREVERB_REVERBTIME_DEF;
		m_Params.fHFRatio = AK_FDNREVERB_HFRATIO_DEF;
		m_Params.uNumberOfDelays = AK_FDNREVERB_NUMBEROFDELAYS_DEF;
		m_Params.fDryLevel = powf( 10.f, AK_FDNREVERB_DRYLEVEL_DEF * 0.05f );
		m_Params.fWetLevel = powf( 10.f, AK_FDNREVERB_WETLEVEL_DEF * 0.05f );
		m_Params.fPreDelay = AK_FDNREVERB_PREDELAY_DEF;
		m_Params.uProcessLFE = AK_FDNREVERB_PROCESSLFE_DEF;
		m_Params.uDelayLengthsMode = AK_FDNREVERB_DELAYLENGTHSMODE_DEF;
		// FX instance will use default delay lengths	
        UnlockParams( );
        return AK_Success;
    }
    return SetParamsBlock( in_pParamsBlock, in_ulBlockSize );
}

AKRESULT CAkFDNReverbFXParams::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
    AK_PLUGIN_DELETE( in_pAllocator, this );
    return AK_Success;
}

// Blob set.
AKRESULT CAkFDNReverbFXParams::SetParamsBlock(	void * in_pParamsBlock, 
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
	AkUInt8 * pucParamPtr = (AkUInt8 *)in_pParamsBlock;
	m_Params.fReverbTime = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fReverbTime );
	m_Params.fHFRatio = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fHFRatio );
	m_Params.uNumberOfDelays = *((AkUInt32 *)pucParamPtr);
	pucParamPtr += sizeof( m_Params.uNumberOfDelays );
	m_Params.fDryLevel = powf( 10.f, AK::ReadBankReal32( (AkReal32 *) pucParamPtr ) * 0.05f );
	pucParamPtr += sizeof( m_Params.fDryLevel );
	m_Params.fWetLevel = powf( 10.f, AK::ReadBankReal32( (AkReal32 *) pucParamPtr ) * 0.05f );
	pucParamPtr += sizeof( m_Params.fWetLevel );
	m_Params.fPreDelay = AK::ReadBankReal32( (AkReal32 *) pucParamPtr );
	pucParamPtr += sizeof( m_Params.fPreDelay );
	m_Params.uProcessLFE = *((bool *)pucParamPtr) ? 1 : 0; 
	pucParamPtr += sizeof( bool ); // Increment by size written in bank (bool)
	m_Params.uDelayLengthsMode = *((AkUInt32 *)pucParamPtr);
	pucParamPtr += sizeof( m_Params.uDelayLengthsMode );
	// In default mode, FX instance will use default delay lengths
	if ( m_Params.uDelayLengthsMode == AKDELAYLENGTHSMODE_CUSTOM ) 
	{
		// Use delay parameters provide
		for ( unsigned int i = 0; i < m_Params.uNumberOfDelays; ++i )
		{
			m_Params.fDelayTime[i] = AK::ReadBankReal32( (AkReal32 *) pucParamPtr ); 
			pucParamPtr += sizeof( m_Params.fDelayTime[i] );
		}
	}
	assert( m_Params.uDelayLengthsMode == AKDELAYLENGTHSMODE_CUSTOM || m_Params.uDelayLengthsMode == AK_FDNREVERB_DELAYLENGTHSMODE_DEF );
	assert( (pucParamPtr - (AkUInt8 *)in_pParamsBlock ) == in_ulBlockSize );

	UnlockParams( );

    return AK_Success;
}

// Update one parameter.
AKRESULT CAkFDNReverbFXParams::SetParam(	AkPluginParamID in_ParamID,
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
	case AK_FDNREVERBFXPARAM_REVERBTIME_ID:	
		m_Params.fReverbTime = *((AkReal32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_HFRATIO_ID:
		m_Params.fHFRatio = *((AkReal32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_NUMBEROFDELAYS_ID:	
		m_Params.uNumberOfDelays = *((AkUInt32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_DRYLEVEL_ID:
		m_Params.fDryLevel = powf( 10.f, *((AkReal32*)in_pValue) * 0.05f );
		break;
	case AK_FDNREVERBFXPARAM_WETLEVEL_ID:	
		m_Params.fWetLevel = powf( 10.f, *((AkReal32*)in_pValue) * 0.05f );
		break;
	case AK_FDNREVERBFXPARAM_PREDELAY_ID:
		m_Params.fPreDelay = *((AkReal32*)in_pValue);
		break;
	case AK_FDNREVERBFXPARAM_PROCESSLFE_ID:
		m_Params.uProcessLFE = *((bool*)in_pValue) ? 1 : 0;
		break;
	case AK_FDNREVERBFXPARAM_DELAYLENGTHSMODE_ID:
		m_Params.uDelayLengthsMode = *((AkUInt32*)in_pValue);
		break;
	default:
		// Note: These may be ignored by FX instance in default delay mode
		assert( in_ParamID >= AK_FDNREVERBFXPARAM_FIRSTDELAYTIME_ID && in_ParamID <= AK_FDNREVERBFXPARAM_LASTDELAYTIME_ID );
		m_Params.fDelayTime[in_ParamID-AK_FDNREVERBFXPARAM_FIRSTDELAYTIME_ID] = *((AkReal32*)in_pValue);
	}

	UnlockParams( );
	return eResult;

}
