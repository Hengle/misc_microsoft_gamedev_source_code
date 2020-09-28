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
// AkChildCtx.h
//
// Base class for all child contexts (playback instances).
// Child contexts are contexts that can be enqueued in a high-level
// context. 
// Defines interface for child playback commands, commands that are
// propagated through the context hierarchy.
// NOTE Currently this is just an interface.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CHILD_CTX_H_
#define _CHILD_CTX_H_

#include "PrivateStructures.h"

class CAkMusicCtx;

class CAkChildCtx
{
public:

	// Context commands
	//

    CAkChildCtx( CAkMusicCtx * in_pParentCtx );

    // Start context playback, propagated from a high-level context _Play().
    // Note: Implementations must support fading in "pre stopped" contexts.
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnPlayed(
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
		) = 0;

	// Stop context playback, propagated from a high-level context Stop().
	//Return - AKRESULT - AK_Success if succeeded
	virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        ) = 0;

	// Pause context playback, propagated from a high-level context _Pause().
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnPaused() = 0;

	// Resume context playback, propagated from a high-level context _Resume().
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnResumed() = 0;

    // Fade management. Set fade level ratios.
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkUInt8 in_uFadeRatio
        ) = 0;

    CAkMusicCtx * Parent() { return m_pParentCtx; }

	CAkChildCtx *		pNextLightItem;	// Sibling. Must be public for ListBareLight.

protected:
    virtual ~CAkChildCtx();
    
    void Connect();

protected:
    CAkMusicCtx *       m_pParentCtx;
};

#endif // _CHILD_CTX_H_
