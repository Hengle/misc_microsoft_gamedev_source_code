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
// AkSource.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkSource.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkFXMemAlloc.h"
#include "AkBankMgr.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkURenderer.h"
#include "AkBankMgr.h"

#define SOURCE_DEFAULT_FILE_FORMAT					eFileWAV	//eFileInvalidFormat
#define SOURCE_DEFAULT_CHANNELSMASK					AK_SPEAKER_SETUP_MONO
#define SOURCE_DEFAULT_BITSPERSAMPLE				AK_LE_NATIVE_BITSPERSAMPLE
#define SOURCE_DEFAULT_BLOCKALIGN					(SOURCE_DEFAULT_BITSPERSAMPLE/8)
#define SOURCE_DEFAULT_TYPEID						AK_INT
#define SOURCE_DEFAULT_INTERLEAVEID					AK_INTERLEAVED
#define SOURCE_DEFAULT_DATAOFFSET					0
#define SOURCE_DEFAULT_DATASIZE						0

#define SOURCE_DEFAULT_TYPE							SrcTypeNone

CAkSource::CAkSource()
:m_bHasSource( false )
{
	SetDefaultParams();
}

void CAkSource::SetDefaultParams()
{
	// Default source type.
	m_sSrcTypeInfo.dwID             = AK_INVALID_SOURCE_ID;
	m_sSrcTypeInfo.pvSrcDesc		= NULL;
	m_sSrcTypeInfo.ulSrcDescSize	= 0;
    m_sSrcTypeInfo.uStreamingLookAhead = 0;

	m_sMediaFormat.SetAll( AK_CORE_SAMPLERATE,
							SOURCE_DEFAULT_CHANNELSMASK,
							SOURCE_DEFAULT_BITSPERSAMPLE,
							SOURCE_DEFAULT_BLOCKALIGN,
							SOURCE_DEFAULT_TYPEID,
							SOURCE_DEFAULT_INTERLEAVEID);

	m_sSrcTypeInfo.mediaInfo.SetDefault( SOURCE_DEFAULT_TYPE );
}

CAkSource::~CAkSource()
{
	FreeSource();
}

void CAkSource::SetSource( AkUInt32 in_PluginID, AkLpCtstr in_pszFilePath, AkAudioFormat in_AudioFormat )
{
	SetMediaFormat( in_AudioFormat );
	FreeSource();

	m_sSrcTypeInfo.dwID				= in_PluginID;
	m_sSrcTypeInfo.pvSrcDesc		= NULL;
	m_sSrcTypeInfo.ulSrcDescSize	= 0;
    m_sSrcTypeInfo.uStreamingLookAhead = 0;

	if( in_pszFilePath != NULL )
	{
		AkUInt32 l_uiLen = (AkUInt32)wcslen(in_pszFilePath);
		l_uiLen = l_uiLen * sizeof( wchar_t ) + sizeof( wchar_t );

		m_sSrcTypeInfo.pvSrcDesc = AkAlloc( SOUND_BUFFER_POOL_ID, l_uiLen );
		AKASSERT( m_sSrcTypeInfo.pvSrcDesc != NULL );

		if( m_sSrcTypeInfo.pvSrcDesc != NULL )
		{
			AKPLATFORM::AkMemCpy( m_sSrcTypeInfo.pvSrcDesc, (void *)in_pszFilePath, l_uiLen );			 
			m_sSrcTypeInfo.ulSrcDescSize = l_uiLen;
		}
	}

	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeFile );
	m_bHasSource = true;
}

void CAkSource::SetSource( AkUInt32 in_PluginID, AkMediaInformation in_MediaInfo, AkAudioFormat in_AudioFormat )
{
	SetMediaFormat( in_AudioFormat );
	FreeSource();

	m_sSrcTypeInfo.dwID				= in_PluginID;
	m_sSrcTypeInfo.pvSrcDesc		= NULL;
	m_sSrcTypeInfo.ulSrcDescSize	= 0;
    m_sSrcTypeInfo.uStreamingLookAhead = 0;

	m_sSrcTypeInfo.mediaInfo = in_MediaInfo;
	m_bHasSource = true;
}

void CAkSource::SetSource( AkPluginID	 	in_ulID,
							void * 	    	in_pParam,
							AkUInt32 		in_uSize )
{
	FreeSource();

	m_sSrcTypeInfo.dwID          = in_ulID;
    AKASSERT( in_pParam != NULL || in_uSize == 0 );              
	m_sSrcTypeInfo.pvSrcDesc     = NULL;
	m_sSrcTypeInfo.ulSrcDescSize = 0;
    m_sSrcTypeInfo.uStreamingLookAhead = 0;

    AKRESULT l_eResult = AK_Fail;
	// Create effect parameters object if data is given.
    if ( in_uSize > 0 && in_pParam != NULL )
    {        
		AK::IAkPluginParam * l_pEffectParam = NULL;

		l_eResult = CAkEffectsMgr::AllocParams( AkFXMemAlloc::GetUpper(), m_sSrcTypeInfo.dwID, l_pEffectParam );

		if( l_eResult == AK_Success && l_pEffectParam != NULL )
		{
            l_eResult = l_pEffectParam->Init( AkFXMemAlloc::GetUpper(), in_pParam, in_uSize );
			
            // Handle plugin initialisation failures.
            if ( l_pEffectParam == NULL || l_eResult != AK_Success )
            {
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );
            }

            // Store in source info.
            m_sSrcTypeInfo.pvSrcDesc     = l_pEffectParam;
			m_sSrcTypeInfo.ulSrcDescSize = sizeof(l_pEffectParam);
		}
    }

	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeModelled );
	m_bHasSource = true;
}


//-----------------------------------------------------------------------------
// Name: SetSrcParam()
// Desc: Set the parameter of a physical model source.
//
// Parameters: AkPluginParamID in_ulParamID: Id of parameter to set.
//			   void *	   in_vpParam:	 Pointer to parameter block.
//             AkUInt32	   in_ulSize:	 Size of the parameter block.
//
// Return: Ak_Success:	Parameter set correctly.
//		   AK_Fail:		Failed to set parameter.
//-----------------------------------------------------------------------------
AKRESULT CAkSource::SetSrcParam(			// Set the parameter on an physical model source.
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *		in_vpParam,			// Pointer to a setup param block.
		AkUInt32			in_ulSize			// Size of the parameter block.
		)
{
	AKRESULT l_eResult = AK_Success;

	if( m_sSrcTypeInfo.dwID != in_ID )
		return AK_Fail;

	if( m_sSrcTypeInfo.pvSrcDesc == NULL )
	{
		AK::IAkPluginParam * l_pEffectParam = NULL;

		l_eResult = CAkEffectsMgr::AllocParams( AkFXMemAlloc::GetUpper(), m_sSrcTypeInfo.dwID, l_pEffectParam );

        AKASSERT( l_eResult != AK_Success || l_pEffectParam != NULL );
		if( l_eResult == AK_Success && l_pEffectParam != NULL )
		{
			m_sSrcTypeInfo.pvSrcDesc     = l_pEffectParam;
			m_sSrcTypeInfo.ulSrcDescSize = sizeof(l_pEffectParam);

            l_eResult = l_pEffectParam->Init( AkFXMemAlloc::GetUpper( ), NULL, 0 );
			AKASSERT( l_eResult == AK_Success );

			// Handle plugin initialisation failures.
			if ( l_pEffectParam == NULL || l_eResult != AK_Success )
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );
			}
		}
	}

	if( l_eResult                   == AK_Success &&
		m_sSrcTypeInfo.pvSrcDesc    != NULL && 
		m_sSrcTypeInfo.ulSrcDescSize > 0 )
	{
		AK::IAkPluginParam * l_pEffectParam = reinterpret_cast<AK::IAkPluginParam*>(m_sSrcTypeInfo.pvSrcDesc);
		l_eResult = l_pEffectParam->SetParam( in_ulParamID, in_vpParam, in_ulSize );
		AKASSERT( l_eResult == AK_Success );

		l_eResult = CAkURenderer::SetParam( l_pEffectParam, in_ulParamID, in_vpParam, in_ulSize );
		AKASSERT( l_eResult == AK_Success );
	}

	return l_eResult;
}

void CAkSource::FreeSource()
{
	if( m_sSrcTypeInfo.pvSrcDesc != NULL )
    {
        switch( m_sSrcTypeInfo.mediaInfo.Type )
        {
            case SrcTypeFile:
			case SrcTypeMemory:
                AkFree( SOUND_BUFFER_POOL_ID, m_sSrcTypeInfo.pvSrcDesc );
                break;

            case SrcTypeModelled:
				AKVERIFY( reinterpret_cast<AK::IAkPluginParam*>(m_sSrcTypeInfo.pvSrcDesc)->Term( AkFXMemAlloc::GetUpper( ) ) == AK_Success );
                break;

            default:
                // Type NONE??
                AKASSERT( !"Not implemented" );
        }
    }

	m_sSrcTypeInfo.mediaInfo.SetDefault( SrcTypeNone );
	m_sSrcTypeInfo.pvSrcDesc = NULL;
	m_bHasSource = false;
}

//-----------------------------------------------------------------------------
// Name: SetMediaFormat()
// Desc: File parser that reads the file header fills in the AkAudioFormat
//           struct and calls this function.
//
//Parameters:
//	AkAudioFormat & in_rMediaFormat : audio format to set.
//-----------------------------------------------------------------------------
void CAkSource::SetMediaFormat( AkAudioFormat & in_rMediaFormat )
{
	m_sMediaFormat = in_rMediaFormat;
}

//-----------------------------------------------------------------------------
// Name: BANK ACCESS INTERFACE : LockDataPtr()
// Desc: Gives access to bank data pointer. Pointer should be released.
// TODO: Implement reference counting, to prevent bank unloading.
//
// Parameters:
//	void *& out_ppvBuffer		: Returned pointer to data.
//	AkUInt32 * out_ulSize		: Size of data returned.
//-----------------------------------------------------------------------------
void CAkSource::LockDataPtr( void *& out_ppvBuffer, AkUInt32 & out_ulSize, CAkPBI* in_pPBI )
{
	AkAutoLock<CAkLock> gate( g_pBankManager->m_BankListLock );

	AkMediaInfo mediaInfo = g_pBankManager->GetMedia( m_sSrcTypeInfo.mediaInfo.sourceID, in_pPBI );

	out_ulSize = mediaInfo.uInMemoryDataSize;
	out_ppvBuffer = mediaInfo.pInMemoryData;
}

void CAkSource::UnLockDataPtr()
{
	AkAutoLock<CAkLock> gate( g_pBankManager->m_BankListLock );

	g_pBankManager->ReleaseMedia( m_sSrcTypeInfo.mediaInfo.sourceID );
}

AKRESULT CAkSource::PrepareData()
{
	if( m_sSrcTypeInfo.mediaInfo.uInMemoryMediaSize != 0 )
	{
		return g_pBankManager->LoadSingleMedia( m_sSrcTypeInfo );
	}
	else
	{
		return AK_Success;
	}
}

void CAkSource::UnPrepareData()
{
	if( m_sSrcTypeInfo.mediaInfo.uInMemoryMediaSize != 0 )
	{
		g_pBankManager->ReleaseSingleMedia( m_sSrcTypeInfo.mediaInfo.sourceID );
	}
}
