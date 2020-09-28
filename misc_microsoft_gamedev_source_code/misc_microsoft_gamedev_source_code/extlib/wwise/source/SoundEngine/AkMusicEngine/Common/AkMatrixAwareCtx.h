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

#ifndef _MATRIX_AWARE_CTX_H_
#define _MATRIX_AWARE_CTX_H_

#include "AkMusicCtx.h"

class CAkMatrixSequencer;
class CAkSegmentChain;

class CAkMatrixAwareCtx : public CAkMusicCtx
{
public:
	CAkMatrixAwareCtx(
        CAkMusicCtx *   in_parent = NULL        // Parent context. NULL if this is a top-level context.
        );
    virtual ~CAkMatrixAwareCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_pGameObj,
        UserParams &    in_rUserparams
        );
	// Called by parent (switches): completes the initialization.
	// Default implementation does nothing.
	virtual void EndInit();

    // Sequencer access.
    virtual void Process(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		) = 0;
    virtual void ProcessRelative(
        AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
        ) = 0;

    // Derived classes access.
    inline CAkMatrixSequencer * Sequencer() { return m_pSequencer; }

    virtual CAkSegmentChain * GetActiveChain() = 0;
	// Returns true if the context implements an empty chain.
	virtual bool IsNullCtx();

    // For Music Renderer's music contexts look-up: concrete contexts must return their own node.
    virtual CAkMusicNode * Node() = 0;

	// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
	virtual void PerformDelayedSwitchChange();

	// Query for transition reversal. Default implementation: true.
	virtual bool CanRestartPlaying() = 0;
	// Revert playback commands that were disabled. Only possible if it can restart playing.
	// Default implementation does nothing.
	virtual void RestartPlaying() = 0;

#ifndef AK_OPTIMIZED
	virtual void OnPaused();
	virtual void OnResumed();
#endif

	bool IsTopLevel() { return ( NULL == m_pParentCtx ); }

//protected:
    // Shared Segment Sequencer.
    // TODO (LX) Enforce do not set sequencer elsewhere than from Music Renderer. Enforce const.
    void SetSequencer( 
        CAkMatrixSequencer * in_pSequencer
        );

private:
    // Shared segment sequencer.
    CAkMatrixSequencer *   m_pSequencer;

	// ListBare usage reserved
public:
	CAkMatrixAwareCtx *	pNextTopLevelSibling;
	
};

#endif //_MATRIX_AWARE_CTX_H_
