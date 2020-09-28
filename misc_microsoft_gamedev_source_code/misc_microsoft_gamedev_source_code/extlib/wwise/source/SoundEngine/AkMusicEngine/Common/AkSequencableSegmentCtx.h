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

#ifndef _SEQUENCABLE_SEGMENT_CTX_H_
#define _SEQUENCABLE_SEGMENT_CTX_H_

#include "AkMatrixAwareCtx.h"
#include "AkSegmentChain.h"

class CAkSequencableSegmentCtx : public CAkMatrixAwareCtx
                                ,public CAkSegmentChain
{
public:
    CAkSequencableSegmentCtx(
        CAkMusicSegment *   in_pSegmentNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSequencableSegmentCtx();

    AKRESULT Init( 
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams,
        CAkSegmentBucket *& out_pFirstBucket    // First (relevant) bucket.
        );

    // Matrix Aware Context implementation.
    // ----------------------------------------------------------

    // Sequencer access.
    virtual void Process(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    virtual void ProcessRelative(
        AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );

    virtual CAkSegmentChain * GetActiveChain();

	// Query for transition reversal. Default implementation: true.
	virtual bool CanRestartPlaying();
	// Revert playback commands that were disabled. Only possible if it can restart playing.
	// Default implementation does nothing.
	virtual void RestartPlaying();

    // For Music Renderer's music contexts look-up: concrete contexts must return their own node.
    virtual CAkMusicNode * Node();

    // Override CAkMusicCtx OnStopped(): Need to unregister ourselves to sequencer.
    virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );

    // CAkSegmentChain implementation
    // ----------------------------------------------------------
    virtual CAkMatrixAwareCtx * MatrixAwareCtx();
    // ----------------------------------------------------------

private:
    // Note. Duplicates its "active" context's segment.
    CAkMusicSegment *   m_pSegmentNode;
};

#endif // _SEQUENCABLE_SEGMENT_CTX_H_
