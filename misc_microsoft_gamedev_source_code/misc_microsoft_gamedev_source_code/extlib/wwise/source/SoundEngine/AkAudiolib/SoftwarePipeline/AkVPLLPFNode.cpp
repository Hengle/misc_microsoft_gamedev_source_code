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
// AkVPLLPFNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLLPFNode.h"
#include "Ak3DParams.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

void CAkVPLLPFNode::ConsumeBuffer( AkVPLState & io_state )
{
	AKVERIFY( m_LPF.SetLPFPar( m_SoundParams->LPF ) == AK_Success ); 

	// this could be no more data if we're getting the last buffer
#ifdef AK_PS3
	io_state.resultPrevious = io_state.result;
#endif
	if ( io_state.buffer.HasData() )
	{
#ifdef __PPU__
		// this will either start a job or update filter memory
		m_LPF.ExecutePS3(&io_state.buffer, io_state.result);
#else
		m_LPF.Execute( &io_state.buffer );
#endif
	}
}

void CAkVPLLPFNode::ProcessDone( AkVPLState & io_state )
{
#ifdef AK_PS3
	io_state.result = io_state.resultPrevious;
#endif
}

AKRESULT CAkVPLLPFNode::ProcessBuffer( AkPipelineBuffer * io_pBuffer, AkReal32 in_fLPFValue )
{
#ifdef __PPU__
	return AK_Fail;
#else
	AKRESULT l_eResult = AK_Success;

	if( io_pBuffer->HasData() )
	{
		AKASSERT( io_pBuffer->uValidFrames > 0 );
		AKVERIFY( m_LPF.SetLPFPar( in_fLPFValue ) == AK_Success ); 
		AKVERIFY( m_LPF.Execute( io_pBuffer ) != 0 );
	}
	else
	{
		l_eResult = AK_Fail;
		AKASSERT( !"Failed to return valid data." );
	}

	return l_eResult;
#endif
} //ProcessBuffer

void CAkVPLLPFNode::ReleaseBuffer()
{
	if ( m_pInput ) // Can be NULL when voice starvation occurs in sample-accurate sequences
		m_pInput->ReleaseBuffer();	 
} // ReleaseBuffer

AKRESULT CAkVPLLPFNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// no need to do anything while skipping time here.
	// this is assuming that the output volume at boundaries of 'skipped time' is equivalent
	// to zero, masking any artefact that might arise from not updating the coefficients.

	return m_pInput->TimeSkip( io_uFrames );
}

AKRESULT CAkVPLLPFNode::Init( AkSoundParams *	in_pSoundParams,		// SoundParams.
							  AkAudioFormat *	in_pFormat )				// Format
{
	AKASSERT( in_pFormat    != NULL );
	AKASSERT( in_pSoundParams != NULL );
	AKASSERT( m_SoundParams == NULL );

	m_SoundParams = in_pSoundParams;

	AKRESULT l_eResult = m_LPF.Init( in_pFormat->GetChannelMask() );
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
} // Init

void CAkVPLLPFNode::Term()
{ 
	m_SoundParams = NULL;
	m_LPF.Term(); 
}
