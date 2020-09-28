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

//////////////////////////////////////////////////////////////////////
//
// Music Segment.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_SEGMENT_H_
#define _MUSIC_SEGMENT_H_

#include "AkMusicNode.h"

class CAkMusicCtx;
class CAkSegmentCtx;
class CAkMusicTrack;

class CAkMusicSegment : public CAkMusicNode
{
public:

    // Thread safe version of the constructor.
	static CAkMusicSegment * Create(
        AkUniqueID in_ulID = 0
        );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

	// Return the node category.
	virtual AkNodeCategory NodeCategory();

    virtual AKRESULT CanAddChild(
        CAkAudioNode * in_pAudioNode 
        );

	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

    // Context factory. 
    // Creates a sequencable segment context, usable by switch containers or as a top-level instance.
    // Returns NULL and cleans up if failed.
	virtual CAkMatrixAwareCtx * CreateContext( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams,
        CAkSegmentBucket *& out_pFirstRelevantBucket
        );

    // Creates a simple segment context and addrefs it. Caller needs to release it. 
	// Returns NULL and cleans up if failed.
    CAkSegmentCtx * CreateSegmentCtxAndAddRef( 
        CAkMatrixAwareCtx * in_pParentCtx,
        CAkRegisteredObj * in_GameObject,
        UserParams &  in_rUserparams
        );

    void Duration(
        AkReal64 in_fDuration               // Duration in milliseconds.
        );
#ifndef AK_OPTIMIZED
	void StartPos(
        AkReal64 in_fStartPos               // PlaybackStartPosition.
        );

    AkUInt32 StartPos() { return m_uStartPos; };
#endif

	AKRESULT SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                 in_ulNumMarkers
		);

    void RemoveMarkers();


    // Interface for Contexts
    // ----------------------

    AkUInt16 NumTracks() { return Children(); }
    CAkMusicTrack * Track(
        AkUInt16 in_uIndex
        );

    // Returns the segment's total duration in samples (at the native sample rate).
    AkUInt32 Duration() { return m_uDuration; };

    // Music transition query. 
    // Returns the delta time (in samples) between the position supplied and the exit Sync point.
    // Minimum time supplied must be relative to the Entry Marker.
    // If no possible exit point is found the method returns AK_Fail. AK_Success if it was found.
    AKRESULT GetExitSyncPos(
        AkInt32         in_iSrcMinTime,         // Minimal time-to-sync constraint related to source, relative to the Entry Marker.
        AkSyncType      in_eSyncType,           // Sync type.
        bool            in_bDoSkipEntryMarker,  // If true, will not consider Entry marker.
        AkInt32 &       out_iExitSyncPos        // Returned Exit Sync position (always >= 0), relative to the Entry Marker.
        );

    // Returns the position of the entry Sync point defined by the rule.
    // Position is relative to the Entry marker.
    AKRESULT GetEntrySyncPos(
		const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
		AkInt32 in_iDesiredEntryPosition,		// Desired entry position (applies to SameTime rule only).
		AkInt32 & out_iRequiredEntryPosition	// Returned entry position.
        );

    // Returns the duration of the pre-entry.
    AkInt32 PreEntryDuration();  // Note. Returns an int because sometimes used with unary operator-.

    // Returns the duration of the post exit.
    AkUInt32 PostExitDuration();

    // Returns the active duration, the length between the Entry and Exit markers.
    AkInt32 ActiveDuration();

    /*
    // Returns true if at least one node of the hierarchy is registered to the state group and has a sync that is
    // not immediate.
    bool DoesRequireDelayedStateChange(
        AkStateGroupID  in_stateGroupID
        );
    */

    bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

protected:
    CAkMusicSegment( 
        AkUniqueID in_ulID
        );
    virtual ~CAkMusicSegment();
    AKRESULT Init();
    void Term();

    // Helpers.
    
    // Return marker positions, relative to the Entry marker.
    AkInt32 ExitMarkerPosition();

    // Find next Bar, Beat, Grid absolute position.
    // Returns AK_Success if a grid position was found before the Exit Marker.
    // NOTE This computation is meant to change with compound time signatures.
    AKRESULT GetNextMusicGridValue(
        AkInt32  in_iPosition,      // Start search position (in samples), relative to Entry marker.
        AkUInt32 in_uGridDuration,  // Grid (beat, bar) duration.
        AkUInt32 in_uOffset,        // Grid offset.
        AkInt32 & out_iExitPosition // Returned position (relative to EntryMarker).
        );
    
    // Find position of first marker after supplied in_iPosition.
    AkInt32 GetNextMarkerPosition(
        AkInt32 in_iPosition,        // Position from which to start search, relative to Entry marker (>=0).
        bool    in_bDoSkipEntryMarker// If true, will not consider Entry marker.
        );
    // Find position of first marker after supplied in_iPosition, excluding Entry and Exit marker.
    // Note. Might fail if there are no user marker before Exit marker.
    AKRESULT GetNextUserMarkerPosition(
        AkInt32 in_iPosition,       // Position from which to start search, relative to Entry marker (may be negative).
        AkInt32 & out_iMarkerPosition
        );
    
    // Finds a user marker by its ID.
    AkUInt32 GetUserMarkerPosition( 
        AkMusicMarkerID in_markerID // Marker ID to search.
        );

private:
    // Array of markers. Markers are stored by their position (in samples).
    // The first marker (index 0) is always the entry marker.
    // The last marker is always the exit marker.
    typedef AkArray<AkMusicMarker,const AkMusicMarker&,ArrayPoolDefault,DEFAULT_POOL_BLOCK_SIZE/sizeof(AkMusicMarker)> MarkersArray;
    MarkersArray    m_markers;

    // Segment duration (in samples at the native sample rate).
    AkUInt32    m_uDuration;

#ifndef AK_OPTIMIZED
	AkUInt32	m_uStartPos;
#endif
};

#endif //_MUSIC_SEGMENT_H_
