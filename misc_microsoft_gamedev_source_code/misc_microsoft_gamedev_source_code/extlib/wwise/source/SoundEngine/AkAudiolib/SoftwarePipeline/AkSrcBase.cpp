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
// AkSrcBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkSrcBase.h"

CAkSrcBaseEx::CAkSrcBaseEx( CAkPBI * in_pCtx )
	: CAkVPLSrcNode( in_pCtx )
	, m_markers( in_pCtx )
	, m_uPCMLoopStart( 0 )
	, m_uPCMLoopEnd( 0 )
{
	m_uLoopCnt = m_pCtx->GetLooping();
}

CAkSrcBaseEx::~CAkSrcBaseEx()
{
}

void CAkSrcBaseEx::StopStream()
{
	m_markers.Free();
}

void CAkSrcBaseEx::CopyRelevantMarkers( 
	AkPipelineBuffer & io_buffer, 
	AkUInt32 in_ulBufferStartPos,
	bool in_bDidLoop 
	)
{
	m_markers.CopyRelevantMarkers( 
		m_pCtx,
		io_buffer, 
		in_ulBufferStartPos,
		m_uPCMLoopStart,
		m_uPCMLoopEnd,
		in_bDidLoop );
}

void CAkSrcBaseEx::NotifyRelevantMarkers( AkUInt32 in_uStartSample, AkUInt32 in_uEndSample )	// Region: [in_uStartSample, in_uEndSample[
{
	m_markers.NotifyRelevantMarkers( m_pCtx, in_uStartSample, in_uEndSample );
}

void CAkSrcBaseEx::UpdatePositionInfo( AkUInt32 in_uStartPos, AkUInt32 in_uFileEnd )
{
	if ( m_markers.NeedPositionInformation() )
		m_markers.UpdatePositionInfo( m_pCtx, 1.0f, in_uStartPos, in_uFileEnd );
}

void CAkSrcBaseEx::TimeSkipMarkers( AkUInt32 in_ulCurrSampleOffset, AkUInt32 in_uSkippedSamples, AkUInt32 in_uFileEnd ) // Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[
{
	m_markers.TimeSkipMarkers( m_pCtx, 1.0f, in_ulCurrSampleOffset, in_uSkippedSamples, in_uFileEnd );
}
