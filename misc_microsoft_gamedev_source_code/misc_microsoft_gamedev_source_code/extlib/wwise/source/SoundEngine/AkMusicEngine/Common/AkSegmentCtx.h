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
// AkSegmentCtx.h
//
// Segment context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _SEGMENT_CTX_H_
#define _SEGMENT_CTX_H_

#include "AkMusicCtx.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkContextualMusicSequencer.h"
#include "AkMonitorData.h"
#include <AK/Tools/Common/AkArray.h>

class CAkMusicSegment;
class CAkSegmentBucket;

class CAkSegmentCtx : public CAkMusicCtx
{
public:
    CAkSegmentCtx(
        CAkMusicSegment *   in_pSegmentNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSegmentCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams
        );

    // Set reference to bucket for OnStopped notification.
    void AttachToBucket(
        CAkSegmentBucket * in_pBucket           // Bucket.
        );
    void DetachFromBucket();

    // Set step track index.
    void StepTrack( 
        AkUInt16            in_uStepIdx         // Step track index.
        );

    // PerformNextFrameBehavior(): 
    // post AudioSources play() cmds based on current position.
    // When this method returns, the position of the segment corresponds to what it should be at the 
    // end of the audio frame that is about to be sinked.
    void PerformNextFrameBehavior(
		AkUInt32 in_uSubframeOffset,	// Offset in frame to start playback enqueueing.
		AkInt32 in_iFrameDuration		// Number of samples to process.
		);

    // Returns current segment's position.
    AkInt32 Position() { return m_sequencer.Now(); }

    // Context commands
	//
    
    // Initialize context for playback.
    // Prepare the context for playback: set initial context position.
    // Audible playback will start at position in_iSourceOffset (relative to EntryCue).
    // The pre-entry will be heard if in_bPlayPreEntry is true.
    // Returns the exact amount of time (samples) that will elapse between call to _Play() and 
    // beginning of playback at position in_iSourceOffset.
	AkInt32 Prepare(
        AkInt32 in_iSourceOffset,           // Position in samples, at the native sample rate, relative to the Entry Cue.
        bool    in_bPlayPreEntry            // True when we require the pre-entry should be heard.
        );

    // Get streaming look-ahead time, as computed by Prepare(). Needed for by Nodes (see note below for m_iStreamingLookAhead).
    AkInt32 GetStreamingLookAheadTime() { return m_iStreamingLookAhead; }

	// Override MusicCtx OnPlayed(): Need to notify for cursor
	virtual void OnPlayed(
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
		);

    // Override MusicCtx OnStopped(): Need to flush actions to release children and to notify for cursor
    virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );

    // Non-virtual counterpart, when user knows it is a segment.
    CAkMusicSegment * SegmentNode() { return m_pSegmentNode; }

    // Used by sequences, when required look-ahead cannot be honored.
    // First few audio sources will not be played back because there is not enough look-ahead.
    void ForceLookAhead(
        AkInt32 in_iSourceOffset,           // Start position in samples, at the native sample rate, relative to the Entry Marker. Must be positive.
        AkInt32 in_iLookAhead               // Allowed look-ahead value (in samples).
        );

    // PBI notifications.
    //

    // Called when PBI destruction occurred from Lower engine without the higher-level hierarchy knowing it.
    // Remove all references to this target from this segment's sequencer.
    void RemoveAllReferences( 
        CAkPBI * in_pCtx   // Context whose reference must be removed.
        );

private:
    void ExecuteInteractiveCmds(
		AkUInt32 in_uSubframeOffset,	// Offset in frame to start playback enqueueing.
		AkInt32 in_iFrameDuration		// Number of samples to process.
		);
	// TODO Enqueue all sources at OnPlayed.
    void ProcessSourcesPlaybackStart(
        AkInt32 in_iAbsPosition,
		AkUInt32 in_uSubframeOffset,	// Offset in frame to start playback enqueueing.
		AkInt32 in_iFrameDuration		// Number of samples to process.
        );
    // Computes context's look-ahead value (samples) according to the position specified.
    AkInt32 ComputeMinSrcLookAhead(
        AkInt32 in_iAbsPosition         // Position in samples (absolute: from beginning of pre-entry).
        );

    // Access to ascendents' shared objects.
    CAkRegisteredObj *  GameObjectPtr();
    AkPlayingID		    PlayingID();
    UserParams &		GetUserParams();

private:

    CAkContextualMusicSequencer m_sequencer;

    CAkMusicSegment *   m_pSegmentNode;    

    CAkSegmentBucket *  m_pBucket;

    // Segment's global streaming look-ahead time. Set by Prepare(). Corresponds to the maximum of all sources'
	// look-ahead.
    AkInt32             m_iStreamingLookAhead;

	void NotifyAction( AkMonitorData::NotificationReason in_eReason );
#ifndef AK_OPTIMIZED

public:
	AkUniqueID GetPlayListItemID(){ return m_PlaylistItemID; }
	void SetPlayListItemID( AkUniqueID in_playlistItemID ){ m_PlaylistItemID = in_playlistItemID; }
private:
	AkUniqueID		m_PlaylistItemID;

#endif

	typedef AkArray<AkUInt16, AkUInt16,ArrayPoolDefault,LIST_POOL_BLOCK_SIZE/sizeof(AkUInt16)> TrackRSArray;
    TrackRSArray    m_arTrackRS;
    AkUInt16        m_bIsInitialScheduling  :1; // True indicates that all sources that are in the range of current
                                                // position must be played (their source offset is deduced).
};

#endif
