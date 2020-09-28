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
// AkVPLFormatConvNode.cpp
//
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkLEngine.h"
#include "AkVPLFormatConvNode.h"
#include "AkProfile.h"

void CAkVPLFormatConvNode::ConsumeBuffer( AkVPLState & io_state )
{
	// In this node, input should not have been queried for data if
	// it already returned AK_NoMoreData. 
	AKASSERT( io_state.buffer.usValidFrames > 0 );

	AkUInt32 uFramesConsumed = 0;

	m_pucData = (AkUInt8*)CAkLEngine::GetCachedAudioBuffer( m_Conv.GetOutNumChannels() * LE_MAX_FRAMES_PER_BUFFER * sizeof( AkReal32 ) ); 
	if ( m_pucData == NULL )
	{
		io_state.result = AK_InsufficientMemory;
		return;
	}

#ifdef __PPU__

	m_Conv.ExecuteSPU( io_state, m_pucData );

#else
	AkAudioBuffer bufferIn = io_state.buffer;

	io_state.buffer.uNumChannels = (AkUInt16) m_Conv.GetOutNumChannels();
	io_state.buffer.usValidFrames = 0;
	io_state.buffer.usMaxFrames = LE_MAX_FRAMES_PER_BUFFER; // Cached audio buffer always allocate max so Ok
	io_state.buffer.pucData = m_pucData;

	uFramesConsumed = m_Conv.Execute( &bufferIn, &io_state.buffer );
	
	// Release size consumed
	m_pInput->ReleaseBuffer();
#endif
}

void CAkVPLFormatConvNode::ProcessDone( AkVPLState & io_state )
{
	AkUInt16 uConsumed = io_state.buffer.usValidFrames;

	// Release size consumed
	m_pInput->ReleaseBuffer();

	io_state.buffer.uNumChannels = (AkUInt16) m_Conv.GetOutNumChannels();
	io_state.buffer.usValidFrames = uConsumed; // assumes we always consume what we get fed
	io_state.buffer.usMaxFrames = LE_MAX_FRAMES_PER_BUFFER; // Cached audio buffer always allocate max so Ok
	io_state.buffer.pucData = m_pucData;

	io_state.result = AK_DataReady;
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLFormatConvNode::ReleaseBuffer()
{
	AKASSERT( m_pInput != NULL );

	// Assume output buffer was entirely consumed by client.
	if( m_pucData != NULL )
	{
		CAkLEngine::ReleaseCachedAudioBuffer( m_Conv.GetOutNumChannels() * LE_MAX_FRAMES_PER_BUFFER * sizeof( AkReal32 ), m_pucData ); 
		m_pucData = NULL;
	}
}

AKRESULT CAkVPLFormatConvNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// no need to do anything while skipping time here.

	return m_pInput->TimeSkip( io_uFrames );
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Init.
//
// Parameters:
//  AkAudioFormat *    in_pFormat    : Format.
// Positioning type.
//
// Return:
//	AK_Success : Init succeeded.
//	AK_Fail    : Buffer could not be created.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFormatConvNode::Init( AkAudioFormat *    io_pFormat,		// Format.
									 bool				bIsFull3D )		// Positioning type.
{
	AKASSERT( io_pFormat	!= NULL );

	m_pucData			= NULL;

	AKRESULT l_eResult = m_Conv.Init( io_pFormat, bIsFull3D );

	return l_eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Term.
//
// Parameters:
//
// Return:
//	AK_Success : Terminated correctly.
//	AK_Fail    : Failed to terminate correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFormatConvNode::Term( )
{
	if( m_pucData != NULL )
	{
		AkFree( g_LEngineDefaultPoolId, m_pucData );
		m_pucData = NULL;
	}

	return m_Conv.Term();
} // Term
