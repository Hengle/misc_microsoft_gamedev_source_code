/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

/////////////////////////////////////////////////////////////////////
//
// AkVPLFilterNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkVPLFilterNode.h"

#include "AkLEngine.h"

#include "AudioLibDefs.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include <AK/SoundEngine/Common/IAkPlugin.h>

void CAkVPLFilterNode::GetBuffer( AkVPLState & io_state )
{
	if( m_bLast )
	{
		io_state.result = AK_NoMoreData;
		ConsumeBuffer( io_state );
	}

	// else just continue up the chain to fetch input buffer
}

void CAkVPLFilterNode::ConsumeBuffer( AkVPLState & io_state )
{
	// Bypass FX if necessary
	const AkFXDesc & l_FxInfo = m_pCtx->GetFX( m_uFXIndex );
	if( l_FxInfo.bIsBypassed || m_pCtx->GetBypassAllFX() )
	{
		if ( !m_LastBypassed )	// Reset FX if not bypassed last buffer
			m_pEffect->Reset( );

		m_LastBypassed = true;

		return;
	}
	else
	{
		m_LastBypassed = false;
	}

	if ( io_state.result == AK_NoMoreData )
		m_bLast = true;
        
	void * pData = NULL;
	if( !io_state.buffer.HasData() )
	{
		AKASSERT( io_state.buffer.MaxFrames() > 0 );
		AKASSERT( m_bAllocatedBuffer == false );

		// Cannot start with a tail thus the request alignment size here is not a problem
		if ( io_state.buffer.GetCachedBuffer( io_state.buffer.MaxFrames(), 
											  m_uChannelMask
											  ) != AK_Success )
		{
			io_state.result = AK_InsufficientMemory;
			return;
		}

		m_bAllocatedBuffer = true;
	}
	else
	{
		m_bAllocatedBuffer = false;
	}

	m_pCurBufferOut = &io_state.buffer;
	m_pCurBufferOut->eState = io_state.result;
	AKASSERT( m_pCurBufferOut->MaxFrames() % 4 == 0 ); // Allocate size for vectorization
	
#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
	m_pEffect->Execute( (AkAudioBuffer*)m_pCurBufferOut, pDspProcess );
	if ( pDspProcess )
	{
		io_state.result = AK_ProcessNeeded;
		CAkLEngine::QueueDspProcess(pDspProcess);
	}
	else
	{
		io_state.result = m_pCurBufferOut->eState;
	}

#else
	AK_START_PLUGIN_TIMER( m_uiID );
 	m_pEffect->Execute( m_pCurBufferOut );
	AK_STOP_PLUGIN_TIMER( m_uiID );
	io_state.result = m_pCurBufferOut->eState;
	AKASSERT( m_pCurBufferOut->uValidFrames <= io_state.buffer.MaxFrames() );	// Produce <= than requested
#endif
}

#ifdef AK_PS3
void CAkVPLFilterNode::ProcessDone( AkVPLState & io_state )
{
	AKASSERT( m_pCurBufferOut->uValidFrames <= io_state.buffer.MaxFrames() );	// Produce <= than requested
	io_state.result = m_pCurBufferOut->eState;
}
#endif

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLFilterNode::ReleaseBuffer()
{
	AKASSERT( m_pInput != NULL );

	// Assume output buffer was entirely consumed by client.
	if( m_bAllocatedBuffer )
	{
		AKASSERT( m_pCurBufferOut );
		m_pCurBufferOut->ReleaseCachedBuffer();
		m_bAllocatedBuffer = false;
	}
	m_pCurBufferOut = NULL;

	m_pInput->ReleaseBuffer();
} // ReleaseBuffer

AKRESULT CAkVPLFilterNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// do nothing while skipping time here.
	// this is fine for non-time-based plugins. 

    if ( m_bLast )
        return AK_NoMoreData;
	return m_pInput->TimeSkip( io_uFrames );
}

void CAkVPLFilterNode::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
		m_pEffect->Reset();

	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		// WG-5935 
		// If the last buffer was drained and we restart from beginning, the flag must be resetted.
		m_bLast = false;
	}
	
    if ( !m_bLast )
    {
		m_pInput->VirtualOn( eBehavior );
	}
}

AKRESULT CAkVPLFilterNode::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
	if ( !m_bLast )
	    return m_pInput->VirtualOff( eBehavior );

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//
// Parameters:
// AkUInt32				in_ulIdx	   : Index into context fx array.
// AkPluginID			in_ID	  	   : Effect id.
// CAkPBI *	in_pCtx		   : Context.
// CAkEffectsMgr *		in_pFxMgr	   : Effects manager.
// AK::IAkDataAccess *  in_pDataAccess : Bank access interface.
// AK::IAkPluginParam * in_pParams     : Parameter interface.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFilterNode::Init( 
		const AkFXDesc & in_fxDesc,
		AkUInt32 in_uFXIndex,
		CAkPBI * in_pCtx,
		AkAudioFormat &	in_format )
{
	AKASSERT( in_pCtx		 != NULL );
	AKASSERT( in_fxDesc.pParam != NULL );

	m_pCtx						 = in_pCtx;
	m_pEffect					 = NULL;
	m_pInsertFXContext			 = NULL;
	m_bLast						 = false;
	m_pCurBufferOut				 = NULL;
	m_LastBypassed				 = false;
	m_bAllocatedBuffer           = false;
	m_uChannelMask				 = in_format.GetChannelMask();
	m_uFXIndex					 = in_uFXIndex;

#ifndef AK_OPTIMIZED
	m_uiID = in_fxDesc.EffectTypeID; // Cached copy of fx id for profiling.
#endif

	AKRESULT l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), in_fxDesc.EffectTypeID, (IAkPlugin*&)m_pEffect );

	if( l_eResult == AK_Success )
	{
		AKASSERT( m_pEffect != NULL );
		AkPluginInfo PluginInfo;
		m_pEffect->GetPluginInfo( PluginInfo );

		if ( PluginInfo.bIsAsynchronous != 
#ifdef AK_PS3
			true
#else
			false
#endif
			)
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 	
			return AK_Fail;
		}

		AK_INCREMENT_PLUGIN_COUNT( in_fxDesc.EffectTypeID );
		m_pInsertFXContext = AkNew( g_LEngineDefaultPoolId, CAkInsertFXContext( in_pCtx, in_uFXIndex ) );
		if ( m_pInsertFXContext != NULL )
		{
			l_eResult = m_pEffect->Init( AkFXMemAlloc::GetLower(),	// Memory allocator.
										 m_pInsertFXContext,		// FX Context.
										 in_fxDesc.pParam,			// Param object.
										 in_format					// Audio format.
										 );

			if ( l_eResult == AK_UnsupportedChannelConfig )
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration );	
			}
			
			if ( l_eResult == AK_Success )
			{
				l_eResult = m_pEffect->Reset( );
			}
			else
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );	
			}
		}
		else
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );
			return AK_Fail;
		}
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
	}

	return l_eResult;
} // Init

void CAkVPLFilterNode::Term()
{
	ReleaseMemory();

	if ( m_pInsertFXContext )
	{
		AkDelete( g_LEngineDefaultPoolId, m_pInsertFXContext );
		m_pInsertFXContext = NULL;
	}
} // Term

void CAkVPLFilterNode::ReleaseMemory()
{
	if( m_pEffect != NULL )
	{
		m_pEffect->Term( AkFXMemAlloc::GetLower() );
		AK_DECREMENT_PLUGIN_COUNT( m_uiID );
		m_pEffect = NULL;
	}
	if( m_bAllocatedBuffer )
	{
		AKASSERT( m_pCurBufferOut );
		m_pCurBufferOut->ReleaseCachedBuffer();
		m_bAllocatedBuffer = false;
	}
	m_pCurBufferOut = NULL;
}
