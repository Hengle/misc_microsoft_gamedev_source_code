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

/////////////////////////////////////////////////////////////////////
//
// AkVPLSrcNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLNode.h"
#include "AkVPLSrcNode.h"
#include "AkSrcBankPCM.h"
#include "AkSrcFilePCM.h"
#include "AkSrcPhysModel.h"
#include "AudiolibDefs.h"
#include "math.h"
#include "AkPositionRepository.h"
#include <AK/Plugin/AkVorbisFactory.h>

#ifdef __PPU__
#include "AkSrcFileADPCMPS3.h"
#include "AkSrcBankADPCMPS3.h"
#else
#include "AkSrcFileADPCM.h"
#include "AkSrcBankADPCM.h"
#endif

#include "AkEffectsMgr.h"

#ifdef XBOX360
#include "AkSrcBankXMA.h"
#include "AkSrcFileXMA.h"
#endif

CAkVPLSrcNode::CAkVPLSrcNode( CAkPBI * in_pCtx )
	: m_pCtx( in_pCtx )
	, m_bIOReady( false )
{
	AKASSERT( m_pCtx != NULL );	 
}


// Create a VPLSrcNode of the appropriate type
CAkVPLSrcNode * CAkVPLSrcNode::Create(
	CAkPBI *		   in_pCtx,
	bool             in_bActive )
{
	CAkVPLSrcNode * pSrc = NULL;

	AkSrcDescriptor l_SrcDesc;
	if ( in_pCtx->GetSrcDescriptor( &l_SrcDesc ) == AK_Success ) 
	{
		// Create the source.

		if ( l_SrcDesc.Type == SrcTypeModelled )
		{
			pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcPhysModel( in_pCtx ) );
		}
		else
		{
			switch ( CODECID_FROM_PLUGINID( l_SrcDesc.uiID ) )
			{
			case AKCODECID_ADPCM:
				if ( l_SrcDesc.Type == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileADPCM( in_pCtx ) );
				else if( l_SrcDesc.Type == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankADPCM( in_pCtx ) );
				break;

#ifdef XBOX360
			case AKCODECID_XMA:
				if ( l_SrcDesc.Type == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFileXMA( in_pCtx ) );
				else if( l_SrcDesc.Type == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankXMA( in_pCtx ) );
				break;
#endif

			case AKCODECID_PCM:
				if ( l_SrcDesc.Type == SrcTypeFile )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcFilePCM( in_pCtx ) );
				else if( l_SrcDesc.Type == SrcTypeMemory )
					pSrc = AkNew( g_LEngineDefaultPoolId, CAkSrcBankPCM( in_pCtx ) );
				break;

			case AKCODECID_VORBIS:
				pSrc = (CAkVPLSrcNode*)CAkEffectsMgr::AllocCodec( in_pCtx, l_SrcDesc.uiID ); // This will allocate bank or file source as necessary
				break;
			}
		}
	}

	if ( pSrc == NULL )
		in_pCtx->Destroy( CtxDestroyReasonPlayFailed );

	return pSrc;
} // Create

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate.
//
// Return:
//	AKRESULT
//		AK_Success : terminated successfully.
//		AK_Fail    : failed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Term( AkCtxDestroyReason in_eReason )
{
	AKVERIFY( m_pCtx->Destroy( in_eReason ) == AK_Success );

	StopStream();

	return AK_Success;
} // Term

//-----------------------------------------------------------------------------
// Name: Start
// Desc: Indication that processing will start.
//
// Parameters:
//	None.
//
// Return:
//	AKRESULT 
//		AK_Success : Started successfully.
//		AK_Fail    : Context failed to play.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Start()
{
	// Set the context's duration for crossfades.
	AkTimeMs l_duration = (AkTimeMs)( GetDuration() / powf( 2.0f, ( m_pCtx->GetPitch()/1200.0f ) ) );
	m_pCtx->SetDuration( l_duration );
	AKRESULT l_eResult = m_pCtx->Play();

	return l_eResult;
} // Start

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Indication that processing will stop.
//
// Parameters:
//	None.
//
// Return:
//	AKRESULT 
//		AK_Success : Stop successfully.
//		AK_Fail    : Failed to stop.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Stop()
{
	AKVERIFY( m_pCtx->Stop() == AK_Success );

	return AK_Success;
} // Stop

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Indication that processing will pause.
//
// Parameters:
//	None.
//
// Return:
//	AKRESULT 
//		AK_Success : Pause successfully.
//		AK_Fail    : Failed to pause.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Pause()
{
	AKRESULT l_eResult = m_pCtx->Pause();
	g_pPositionRepository->SetRate( m_pCtx->GetPlayingID(), 0 );
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
} // Pause

//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Indication that processing will resume.
//
// Parameters:
//	None.
//
// Return:
//	AKRESULT 
//		AK_Success : Resume successfully.
//		AK_Fail    : Failed to resume.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Resume( AkReal32 in_fOutputRate )
{
	AKRESULT l_eResult = m_pCtx->Resume();
	g_pPositionRepository->SetRate( m_pCtx->GetPlayingID(), in_fOutputRate );
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
} // Resume

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//
// Return:
//	Ak_Success: Effect input was connected.
//  AK_Fail:    Failed to connect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Connect( CAkVPLNode * in_pInput )
{
	AKASSERT( !"Not implemented" );
	return AK_Fail;
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to disconnect.
//
// Return:
//	Ak_Success: Effect input was disconnected.
//  AK_Fail:    Failed to disconnect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcNode::Disconnect( CAkVPLNode * in_pInput )
{
	AKASSERT( !"Not implemented" );
	return AK_Fail;
} // Disconnect


AKRESULT CAkVPLSrcNode::TimeSkip( AkUInt32 & io_uFrames )
{
	AkVPLState state;
	state.buffer.SetRequestSize( (AkUInt16) io_uFrames );
    GetBuffer( state );
	if ( state.buffer.HasData() )
	{
		io_uFrames = state.buffer.uValidFrames;
		ReleaseBuffer();
	}
	else
	{
		io_uFrames = 0;
	}

	return state.result;
}
