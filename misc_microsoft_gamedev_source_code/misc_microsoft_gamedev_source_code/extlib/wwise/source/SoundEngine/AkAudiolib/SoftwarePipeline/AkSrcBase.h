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
// AkSrcBase.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_SRC_BASE_H_
#define _AK_SRC_BASE_H_

#include "AkCommon.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkPBI.h"
#include "AkVPLSrcNode.h"
#include "AkMarkers.h"

class CAkSrcBaseEx : public CAkVPLSrcNode
{
public:

    CAkSrcBaseEx( CAkPBI * in_pCtx );
	virtual ~CAkSrcBaseEx();

	void CopyRelevantMarkers( 
		AkPipelineBuffer & io_buffer, 
		AkUInt32 in_ulBufferStartPos,
		bool in_bDidLoop = false
		);
	
	// These 3 functions are used for virtual voices only
	void NotifyRelevantMarkers( AkUInt32 in_uStartSample, AkUInt32 in_uEndSample );	// Region: [in_uStartSample, in_uEndSample[
	void UpdatePositionInfo( AkUInt32 in_uStartPos, AkUInt32 in_uFileEnd );
	void TimeSkipMarkers( AkUInt32 in_ulCurrSampleOffset, AkUInt32 in_uSkippedSamples, AkUInt32 in_uFileEnd ); // Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[

	//helper to know if we are going to loop
	bool DoLoop() { return m_uLoopCnt != 1; }

	virtual void StopStream();

protected:

	CAkMarkers			m_markers;

	AkUInt16            m_uLoopCnt;   // Number of remaining loops.
	AkUInt32			m_uPCMLoopStart;
	AkUInt32			m_uPCMLoopEnd;
};

#endif
