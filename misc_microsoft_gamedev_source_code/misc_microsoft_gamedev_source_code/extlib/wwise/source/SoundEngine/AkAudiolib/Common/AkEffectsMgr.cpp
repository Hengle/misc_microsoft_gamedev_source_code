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
// AkEffectsMgr.cpp
//
// Implementation of the effects manager.
// The effects manager takes care of allocating, deallocating and 
// storing the effect instances. 
// The initial effect parameters are kept at the node level. They are
// modified in run-time through RTPC, but the renderer sees them as
// anonymous blocks of data.
// The effects manager lives as a singleton in the AudioLib.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkEffectsMgr.h"
#include "AkFXMemAlloc.h"
#include "AudiolibDefs.h"
#include "AkMonitor.h"
#include "AkPBI.h"
#include "AkAudioMgr.h"
#include "AkFeedbackStructs.h"

//-----------------------------------------------------------------------------
// Variables.
//-----------------------------------------------------------------------------

CAkEffectsMgr::FXList CAkEffectsMgr::m_RegisteredFXList;  // list keyed by the plugin unique type ID.
CAkEffectsMgr::CodecList CAkEffectsMgr::m_RegisteredCodecList;  // list keyed by the codec unique type ID.
CAkEffectsMgr::FeedbackBusList CAkEffectsMgr::m_RegisteredFeedbackBus;// list keyed by the bus unique type ID.

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise lists.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::Init( )
{    
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate device manager.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::Term( )
{
    // Terminate registered FX list.
    m_RegisteredFXList.Term();
	m_RegisteredCodecList.Term();
	m_RegisteredFeedbackBus.Term();
    return AK_Success;
}

// Registration-time

//-----------------------------------------------------------------------------
// Name: RegisterPlugin
// Desc: Registers an effect with its unique ID. Derived CAkRTEffect classes
//       must register themselves in order to be used as plugins.
//       The effect type ID must be unique, otherwise this function will fail.
// Note: This function is NOT thread-safe.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::RegisterPlugin( 
	AkPluginType in_eType,
	AkUInt32 in_ulCompanyID,						// Company identifier (as declared in plugin description XML)
	AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in plugin description XML)
    AkCreatePluginCallback	in_pCreateFunc,			// Pointer to the effect's Create() function.
    AkCreateParamCallback	in_pCreateParamFunc		// Pointer to the effect param's Create() function.
    )
{
	AKASSERT( in_eType );

	AkUInt32 ulPluginID = ( in_eType & AkPluginTypeMask ) 
		+ ( in_ulCompanyID << 4 ) 
		+ ( in_ulPluginID << ( 4 + 12 ) );

	// Verify that the Effect type ID doesn't exist.
    if ( m_RegisteredFXList.Exists( ulPluginID ) )
    {
        // An effect is already registered with this ID.
        return AK_Fail;
    }

    AKASSERT( NULL != in_pCreateFunc );
    AKASSERT( in_pCreateParamFunc != NULL );
    if ( NULL == in_pCreateFunc || in_pCreateParamFunc == NULL )
    {
        return AK_InvalidParameter;
    }

    EffectTypeRecord NewTypeRecord = { in_pCreateFunc, in_pCreateParamFunc };
    
	return m_RegisteredFXList.Set( ulPluginID, NewTypeRecord ) ? AK_Success : AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: RegisterCodec
// Desc: Registers a codec with its unique ID. 
// Note: This function is NOT thread-safe.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::RegisterCodec( 
		AkUInt32 in_ulCompanyID,						// Company identifier (as declared in XML)
		AkUInt32 in_ulPluginID,							// Plugin identifier (as declared in XML)
        AkCreateFileSourceCallback in_pFileCreateFunc,  // File source.
        AkCreateBankSourceCallback in_pBankCreateFunc   // Bank source.
    )
{
	AkUInt32 ulPluginID = ( AkPluginTypeCodec & AkPluginTypeMask ) 
		+ ( in_ulCompanyID << 4 ) 
		+ ( in_ulPluginID << ( 4 + 12 ) );

	// Verify that the Effect type ID doesn't exist.
    if ( m_RegisteredCodecList.Exists( ulPluginID ) )
    {
        // A codec is already registered with this ID.
        return AK_Fail;
    }

    AKASSERT( in_pFileCreateFunc != NULL );
	AKASSERT( in_pBankCreateFunc != NULL );
    if ( in_pFileCreateFunc == NULL || in_pBankCreateFunc == NULL )
    {
        return AK_InvalidParameter;
    }

    CodecTypeRecord NewTypeRecord = { in_pFileCreateFunc, in_pBankCreateFunc };  
	return m_RegisteredCodecList.Set( ulPluginID, NewTypeRecord ) ? AK_Success : AK_Fail;
}

// Run-time

//-----------------------------------------------------------------------------
// Name: Alloc
// Desc: Effects object factory. The appropriate registered 
//       CreateEffect function is called herein.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::Alloc( 
	AK::IAkPluginMemAlloc * in_pAllocator,
	AkPluginID in_EffectTypeID,                     // Effect type ID.
	AK::IAkPlugin* & out_pEffect                   // Effect instance.
    )
{
    out_pEffect = NULL;

    // See if the effect type is registered.
    EffectTypeRecord * pTypeRec = m_RegisteredFXList.Exists( in_EffectTypeID );
    if ( !pTypeRec )
    {
        MONITOR_ERROR1( AK::Monitor::ErrorCode_PluginNotRegistered, in_EffectTypeID );
        return AK_Fail;
    }

    AKASSERT( pTypeRec->pCreateFunc != NULL );

    // Call the effect creation function.
    out_pEffect = (*pTypeRec->pCreateFunc)( in_pAllocator );

	return out_pEffect ? AK_Success : AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: Alloc
// Desc: Effects object factory. The appropriate registered 
//       CreateEffect function is called herein.
//-----------------------------------------------------------------------------
AKRESULT CAkEffectsMgr::AllocParams(
	AK::IAkPluginMemAlloc * in_pAllocator,
	AkPluginID in_EffectTypeID,               // Effect type ID.
    AK::IAkPluginParam * & out_pEffectParam  // Effect param instance.
    )
{
    out_pEffectParam = NULL;

    // See if the effect type is registered.
    EffectTypeRecord * pTypeRec = m_RegisteredFXList.Exists( in_EffectTypeID );
    if ( !pTypeRec )
    {
        // Plug-in not registered.
		MONITOR_ERROR1( AK::Monitor::ErrorCode_PluginNotRegistered, in_EffectTypeID );
#ifndef AK_OPTIMIZED
		fprintf( stderr,"Wwise warning: Plug-in (ID = %i) not registered\n", in_EffectTypeID );
#endif
        return AK_Fail;
    }

    if ( NULL == pTypeRec->pCreateParamFunc )
    {
        // Registered, but no EffectParam object.
        return AK_Success;
    }

    // Call the effect creation function.
    out_pEffectParam = (*pTypeRec->pCreateParamFunc)( in_pAllocator );

	return out_pEffectParam ? AK_Success : AK_Fail;
}

//-----------------------------------------------------------------------------
// Name: AllocCodec
// Desc: Codec object factory. The appropriate registered creation function is called herein.
//-----------------------------------------------------------------------------
IAkSoftwareCodec * CAkEffectsMgr::AllocCodec( 
		CAkPBI * in_pCtx,						// Source context.
		AkCodecID in_CodecID					// Codec type ID.
    )
{
    // See if the codec type is registered.
    CodecTypeRecord * pTypeRec = m_RegisteredCodecList.Exists( in_CodecID );
    if ( !pTypeRec )
    {
		MONITOR_ERROR1( AK::Monitor::ErrorCode_CodecNotRegistered, in_CodecID );
#ifndef AK_OPTIMIZED
		fprintf( stderr,"Wwise warning: Codec (ID = %i) not registered\n", in_CodecID );
#endif
        return NULL;
    }
	
	IAkSoftwareCodec * pSrc = NULL;

	AkSrcDescriptor SrcDesc;
	if ( in_pCtx->GetSrcDescriptor( &SrcDesc ) == AK_Success ) 
	{
		if ( SrcDesc.Type == SrcTypeFile )
		{
			AKASSERT( pTypeRec->pFileCreateFunc != NULL );
			pSrc = (*pTypeRec->pFileCreateFunc)( (void*)in_pCtx );
		} 
		else
		{
			AKASSERT( pTypeRec->pBankCreateFunc != NULL );
			pSrc = (*pTypeRec->pBankCreateFunc)( (void*)in_pCtx );
		}
	} 

	return pSrc;
}

AKRESULT CAkEffectsMgr::RegisterFeedbackBus(
		AkUInt32 in_iCompanyID,
		AkUInt32 in_iBusPluginID,						// Plugin identifier (as declared in XML)
		AkCreatePluginCallback in_pCreateMixNodeFunc)		// Object creation callback		
{
	AkUInt32 ulPluginID = ( AkPluginTypeMotionDevice & AkPluginTypeMask ) 
		+ ( in_iCompanyID << 4 ) 
		+ ( in_iBusPluginID << ( 4 + 12 ) );

	if (m_RegisteredFeedbackBus.Exists(ulPluginID) != NULL)
		return AK_Fail;

	m_RegisteredFeedbackBus.Set(ulPluginID, in_pCreateMixNodeFunc);
	return AK_Success;
}

AKRESULT CAkEffectsMgr::AllocFeedbackBus(
		AkUInt32 in_iCompanyID,							// Company ID
		AkUInt32 in_iPluginID,							// Bus plugin ID
		AkPlatformInitSettings * io_pPDSettings,		// The global init settings
		AkUInt8 in_iPlayer, 							// The player ID
		IAkMotionMixBus*& out_pMixNode)				// Output bus pointer
{
	AkUInt32 ulPluginID = ( AkPluginTypeMotionDevice & AkPluginTypeMask ) 
		+ ( in_iCompanyID << 4 ) 
		+ ( in_iPluginID << ( 4 + 12 ) );

	AkCreatePluginCallback *pCreateFunc = m_RegisteredFeedbackBus.Exists(ulPluginID);
	if (pCreateFunc != NULL)
	{
		out_pMixNode = static_cast<IAkMotionMixBus*>((*pCreateFunc)(AkFXMemAlloc::GetLower()));
		if (out_pMixNode != NULL)
		{
			if (out_pMixNode->Init(AkFXMemAlloc::GetLower(), io_pPDSettings, in_iPlayer) == AK_Success)
				return AK_Success;
			else	
			{		
				out_pMixNode->Term(AkFXMemAlloc::GetLower());				
				out_pMixNode = NULL;
			}
		}
	}

	return AK_Fail;
}
