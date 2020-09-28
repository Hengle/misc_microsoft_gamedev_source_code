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
// AkMusicCtx.h
//
// Base context class for all parent contexts.
// Propagates commands to its children. Implements child removal.
//
// NOTE: Only music contexts are parent contexts, so this class is
// defined here. Move to AkAudioEngine if other standard nodes use them.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_CTX_H_
#define _MUSIC_CTX_H_

#include "AkChildCtx.h"
#include "AkPoolSizes.h"
#include "AkListBareLight.h"
#include "AkTransition.h"
#include "AkMusicStructs.h"
#include "AkRegisteredObj.h"
#include "IAkTransportAware.h"

class CAkMusicNode;
class CAkMusicSegment;
struct AkMusicTransSrcRule;

#define AK_USE_MIN_TRANSITION_TIME	(-1)	// Default value for _Stop subframe offset. 
// IMPORTANT: Keep in sync with AK_NO_IN_BUFFER_STOP_REQUESTED


class CAkMusicCtx : public CAkObject,
                    public CAkChildCtx,
                    public ITransitionable,
                    public IAkTransportAware
{
public:

    CAkMusicCtx( 
        CAkMusicCtx *   in_parent = NULL        // Parent context. NULL if this is a top-level context.
        );

    virtual ~CAkMusicCtx();

    // Init: Connects to parent.
    void Init(
        CAkRegisteredObj *  in_pGameObj,
        UserParams &    in_rUserparams        
        );

    // Parent-child management.
    void AddChild(
        CAkChildCtx * in_pChildCtx
        );
    void RemoveChild( 
        CAkChildCtx * in_pChildCtx
        );

    // Ref counting interface.
    void AddRef() { ++m_uRefCount; }
    void Release();

    // ITransitionable implementation:
    // ----------------------------------------
    // Fades: Music context keep their own fade level, and register it to the leaf (PBI)'s mute map,
    // by propagating it through the CAkChildCtx::SetPBIFade() interface.
    virtual void TransUpdateValue(
        TransitionTargetTypes in_eTargetType, 
        TransitionTarget in_unionValue, 
        bool in_bIsTerminated 
        );

    // Child context handling implementation.
    // ----------------------------------------

    // Start context playback, propagated from a high-level context _Play().
    // Note: Implementations must support fading in "pre stopped" contexts.
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnPlayed(
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
		);

	// Stop context playback, propagated from a high-level context _Stop().
	//Return - AKRESULT - AK_Success if succeeded
	virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );

	// Pause context playback, propagated from a high-level context _Pause().
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnPaused();

	// Resume context playback, propagated from a high-level context _Resume().
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnResumed();

    // Fade management. Propagate fades down to PBIs muted map
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkUInt8 in_uFadeRatio
        );
    
	// Context commands:
	// Propagate _Cmd() messages to children.
    // ----------------------------------------
    
    // Start context playback.
	// Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Play( 
        AkMusicFade & in_fadeParams,
		AkUInt32 in_uSubFrameOffset	= 0	// Sample offset inside audio frame to actually start playing.
        );

    // Start context playback. No fade offset overload.
	// Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Play( 
        TransParams & in_transParams,
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
        );

	// Stop context playback.
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Stop( 
        TransParams & in_transParams,
		AkUInt32 in_uSubFrameOffset = AK_USE_MIN_TRANSITION_TIME // Sample offset inside audio frame to actually stop playing.
        );

	// Pause context playback.
	// Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Pause( 
        TransParams & in_transParams
        );

	// Resume context playback.
	// Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Resume( 
        TransParams & in_transParams,
        bool in_bIsMasterResume // REVIEW
        );


    // IAkTransportAware interface implementation.
    // Needed for Playing Mgr. NOT IMPLEMENTED.

    // Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false );

#ifndef AK_OPTIMIZED
	virtual AKRESULT _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		);
#endif

    inline bool               IsPlaying() { return m_bIsPlaying; }
    inline bool               IsPaused() { return m_bIsPaused; }

protected:

    // Common members.
	PlaybackTransition	m_PBTrans;      // Context's current playback transition.

    // Parent/child management.
    typedef AkListBareLight<CAkChildCtx> ChildrenCtxList;
    ChildrenCtxList     m_listChildren;

#ifdef _DEBUG
	AkUInt32 NumChildren();
#endif

private:

    // Ref counting.
    AkUInt32            m_uRefCount;

    // Pause count.
    AkUInt16            m_uPauseCount;

    // Fade levels.
    // Note. Even though fade levels are sent directly to leaves through CAkChildCtx::SetPBIFade(), their values are
    // kept instead of just a flag (bAudible) for 2 reasons:
    // 1) we use uStopFade and uPauseFade addresses as the mute map key (distinguish pause and stop);
    // 2) possible optimization for contexts: avoid performing actions (like creating PBIs) if they will not be
    // audible anyway.
    AkUInt8             m_uPlayStopFade;
    AkUInt8             m_uPauseResumeFade;

private:
    AkUInt8             m_bIsPlaying    :1; // True when this context is playing.
    AkUInt8             m_bIsPaused     :1; // True when this context is completely paused.
};

#endif
