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

#ifndef _AK_MARKERS_H_
#define _AK_MARKERS_H_

#include "AkCommon.h"

class CAkPBI;

class CAkMarkers
{
public:

    CAkMarkers( CAkPBI * in_pCtx );
	virtual ~CAkMarkers();
	
	AKRESULT Allocate( AkUInt32 in_uNumMarkers );
	AKRESULT SetLabel( AkUInt32 in_idx, char * in_psLabel, AkUInt32 in_uStrSize );
	void Free();
	
	inline AkUInt32 Count() { return m_hdrMarkers.uNumMarkers; }
	inline bool NeedMarkerNotification() { return ( m_pMarkers && m_bWantMarkerNotifications ); }
	inline bool NeedPositionInformation() { return m_bWantPositionInformation; }
	
	void CopyRelevantMarkers( 
		CAkPBI* in_pCtx,
		AkPipelineBuffer & io_buffer, 
		AkUInt32 in_ulBufferStartPos,
		AkUInt32 in_uPCMLoopStart,
		AkUInt32 in_uPCMLoopEnd,
		bool in_bDidLoop = false,
		const AkUInt32 * in_puForceNumFrames=NULL	// uses this number of frames instead of deducing it from io_buffer.
		);
							
	// Sends markers to PlayingMgr that are located in region [in_uStartSample, in_uStopSample[  (PCM samples) .
	void NotifyRelevantMarkers( 
		CAkPBI * in_pCtx, 
		AkUInt32 in_uStartSample, 
		AkUInt32 in_uStopSample
		);
	
	// IMPORTANT: Do not call if !NeedPositionInformation().
	void UpdatePositionInfo( 
		CAkPBI * in_pCtx, 
		AkReal32 in_fLastRate, 
		AkUInt32 in_uStartPos, 
		AkUInt32 in_uFileEnd		// File size.
		);

	void TimeSkipMarkers( 
		CAkPBI * in_pCtx, 
		AkReal32 in_fLastRate,
		AkUInt32 in_ulCurrSampleOffset, 
		AkUInt32 in_uSkippedSamples,// Region: [in_ulCurrSampleOffset, in_ulCurrSampleOffset+in_uSkippedSamples[
		AkUInt32 in_uFileEnd		// File size.
		);

	static inline AkReal32 GetRate( AkPitchValue in_pitch ) { return powf( 2.f, in_pitch / 1200.f ); }

public:
	AkMarkersHeader     m_hdrMarkers; // Markers header.
    AkAudioMarker *     m_pMarkers;   // Marker data.
protected:
	bool				m_bWantMarkerNotifications;
	bool				m_bWantPositionInformation;
};

#endif // _AK_MARKERS_H_
