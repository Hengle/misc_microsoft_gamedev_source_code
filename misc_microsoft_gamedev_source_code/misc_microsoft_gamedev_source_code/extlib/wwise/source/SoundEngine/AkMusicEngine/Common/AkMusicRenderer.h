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
// AkMusicRenderer.h
//
// Base context class for all parent contexts.
// Propagates commands to its children. Implements child removal.
//
// NOTE: Only music contexts are parent contexts, so this class is
// defined here. Move to AkAudioEngine if other standard nodes use them.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_RENDERER_H_
#define _MUSIC_RENDERER_H_

#include "AkMusicPBI.h"
#include "AkListBare.h"
#include "AkMatrixAwareCtx.h"
#include <AK/MusicEngine/Common/AkMusicEngine.h>

class CAkSoundBase;
class CAkSource;

class CAkMatrixSequencer;




//-------------------------------------------------------------------
// Class CAkMusicRenderer: Singleton. Manages top-level music contexts,
// routes game actions to music contexts, acts as an external Behavioral 
// Extension and State Handler, and as a music PBI factory.
//-------------------------------------------------------------------
class CAkMusicRenderer : public CAkObject
{
public:

    static CAkMusicRenderer * Create(
		AkMusicSettings *	in_pSettings
		);
    inline static CAkMusicRenderer * Get() { return m_pMusicRenderer; }
    void Destroy();

	// Access to global settings.
	inline static AkReal32 StreamingLookAheadRatio() { return m_musicSettings.fStreamingLookAheadRatio; }

    // Add/Remove Top-Level Music Contexts (happens when created from MusicNode::Play()).
    
    AKRESULT AddChild( 
        CAkMatrixAwareCtx * in_pMusicCtx,
        UserParams &    in_rUserparams,
        CAkRegisteredObj *  in_pGameObj
        );
    void RemoveChild( 
        CAkMatrixAwareCtx * in_pMusicCtx
        );
    
    // Behavioral Extension callback.
    static void PerformNextFrameBehavior();

    
    // Similar to URenderer::Play().
    // OPTIMIZE (LX) Remove PBI factory.
    // Creates a Music PBI (a PBI that can be a child of a high-level context) and assigns a parent.
    // Returns it
    static AKRESULT Play( 
        CAkMusicCtx *		io_pParentCtx,
		CAkSoundBase*		in_pSound,
		CAkSource *			in_pSource,
        CAkRegisteredObj *	in_pGameObj,
        TransParams &		in_transParams,
        UserParams&			in_rUserparams,
        AkUInt32			in_uSourceOffset,   // Start position of source (in samples, at the native sample rate).
		AkUInt32			in_uFrameOffset,    // Frame offset for look-ahead and LEngine sample accuracy.
		CAkMusicPBI *&		out_pPBI            // TEMP: Created PBI is needed to set the transition from outside.
        );

    // Game triggered actions (stop/pause/resume).
    AKRESULT Stop(	
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams,
		AkPlayingID			in_playingID = AK_INVALID_PLAYING_ID
        );
	AKRESULT Pause(
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams
        );
	AKRESULT Resume(	
        CAkMusicNode *      in_pNode,
        CAkRegisteredObj *  in_pGameObj,
        TransParams &       in_transParams,
        bool                in_bIsMasterResume  // REVIEW
        );


    // States management.
    // ---------------------------------------------

    // External State Handler callback.
    // Returns true if state change was handled (delayed) by the Music Renderer. False otherwise.
    static bool SetState( 
        AkStateGroupID      in_stateGroupID, 
        AkStateID           in_stateID
        );

    // Execute a StateChange music action.
    void PerformDelayedStateChange(
        void *              in_pCookie
        );
    // Notify Renderer whenever a StateChange music action is Flushed.
    void RescheduleDelayedStateChange(
        void *              in_pCookie
        );
    
#ifndef AK_OPTIMIZED
	// External handle for profiling interactive music specific information.
	static void HandleProfiling();
#endif

private:
    static AKRESULT Play(	
        CAkMusicPBI *   in_pContext, 
        TransParams &   in_transParams
        );

	// Stops all top-level contexts.
	static void StopAll();

    CAkMusicRenderer();
    virtual ~CAkMusicRenderer();
    AKRESULT Init(
		AkMusicSettings *	in_pSettings
		);

private:
    static CAkMusicRenderer * m_pMusicRenderer;
	static AkMusicSettings	m_musicSettings;

    // List of top-level music contexts.
	
	// Structure for top-level list bare.
	struct AkListBareNextTopLevelCtxSibling
	{
		static AkForceInline CAkMatrixAwareCtx *& Get( CAkMatrixAwareCtx * in_pItem ) 
		{
			return in_pItem->pNextTopLevelSibling;
		}
	};
	typedef AkListBare<CAkMatrixAwareCtx,AkListBareNextTopLevelCtxSibling> MatrixAwareCtxList;
    static MatrixAwareCtxList	m_listCtx;

    // States management.
    struct AkStateChangeRecord
    {
        AkStateGroupID  stateGroupID;
        AkStateID       stateID;
		AkUInt32		bWasPosted		:1;
		AkUInt32		bIsReferenced	:1;
    };
    // Note: Order is important. New pending state changes are added at the beginning of the list. Old ones are at the end.
    typedef CAkList2<AkStateChangeRecord,const AkStateChangeRecord&,AkAllocAndFree> PendingStateChanges;
    static PendingStateChanges          m_queuePendingStateChanges;

    // Helpers.
    // Query top-level context sequencers that need to handle state change by delaying it.
    // Returns the minimal absolute delay for state change. Value <= 0 means "immediate".
    static AkInt32 GetDelayedStateChangeData(
        AkStateGroupID          in_stateGroupID, 
        CAkMatrixAwareCtx *&    out_pChosenCtx,
        AkInt32 &               out_iChosenRelativeSyncTime,
        AkUInt32 &              out_uChosenSegmentLookAhead
        );
    typedef PendingStateChanges::Iterator   PendingStateChangeIter;
    typedef PendingStateChanges::IteratorEx PendingStateChangeIterEx;
    static void CancelDelayedStateChange( 
        AkStateGroupID     in_stateGroupID, 
        PendingStateChangeIterEx & in_itPendingStateChg
        );
    static void FindPendingStateChange( 
        void * in_pCookie,
        PendingStateChangeIterEx & out_iterator
        );
    static void InvalidateOlderPendingStateChanges( 
        PendingStateChangeIter & in_iterator,
        AkStateGroupID           in_stateGroupID
        );
    static void CleanPendingStateChanges();

	// Synchronization event for MusicEngine::Term():
	// We need to wait for the Lower Engine to stop/notify all music PBIs
	// before we can destroy our data structures.
	static AkEvent m_hTermEvent;
};

#endif
