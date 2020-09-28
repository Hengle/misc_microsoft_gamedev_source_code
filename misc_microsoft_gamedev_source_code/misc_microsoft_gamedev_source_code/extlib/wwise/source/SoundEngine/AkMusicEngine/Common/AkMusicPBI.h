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
// AkMusicPBI.h
//
// Enhances the PBI by keeping a pointer to a parent (music) context. 
// Removes itself from parent in destructor.
// Also, handles transitions of a multi-level context hierarchy:
// checks if it is registered to a transition of one of its ascendant
// before creating one (a extra step before PBI::_Play/_Stop).
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_PBI_H_
#define _MUSIC_PBI_H_

#include "AkPBI.h"
#include "AkMusicCtx.h"

// IMPORTANT: CAkPBI must be the first base class specified, because the Upper Renderer AkDestroys
// them through a pointer to a CAkPBI.
class CAkMusicPBI : public CAkPBI,
                    public CAkChildCtx
{

    friend class CAkMusicRenderer;

public:

    AKRESULT Init( AkPathInfo* in_pPathInfo );

	virtual void Term();

    
    // Child context implementation.
    // ------------------------------------------

    // Start context playback, propagated from a high-level context _Play().
	// Return - AKRESULT - AK_Success if succeeded
	virtual void OnPlayed(
		AkUInt32	in_uSubFrameOffset	// Sample offset inside audio frame to actually start playing.
		);

	// Stop context playback, propagated from a high-level context Stop().
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

    // Fade management. Set fade level ratios.
    virtual void SetPBIFade( 
        void * in_pOwner,
        AkUInt8 in_uFadeRatio
        );


    // Overriden methods from PBIs.
    // ---------------------------------------------

    // Stop the PBI (the PBI is then destroyed)
	AKRESULT _Stop( 
        AkUInt32 in_uStopOffset
        );

	virtual AKRESULT _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false );

	//Must be overriden to force interactive music to ignore pitch from busses
	virtual AKRESULT		GetParams( AkSoundParams * io_Parameters );
	virtual AkPitchValue	GetPitch();

    // Only the Music Renderer can create/_Play music PBIs.
private:

	virtual AkUInt32	GetSourceOffset();
	virtual AkUInt32	GetAndClearStopOffset();

	virtual void SetSourceOffset( AkUInt32 in_ulSourceOffset ){ m_ulSourceOffset = in_ulSourceOffset; }
	void SetStopOffset( AkUInt32 in_ulStopOffset ){ m_ulStopOffset = in_ulStopOffset; }
    
    CAkMusicPBI(
        CAkMusicCtx *   in_parent,
        CAkSoundBase*	in_pSound,			// Pointer to the sound.
		CAkSource*		in_pSource,			// Pointer to the source.
		CAkRegisteredObj * in_pGameObj,		// Game object and channel association.
		UserParams&		in_UserParams,		// User Parameters.
		PlayHistory&	in_rPlayHistory,	// History stuff.
		AkUniqueID		in_SeqID,			// Sample accurate seq id.
		AkPriority		in_Priority,
		AkUInt32		in_uSourceOffset
        );

    virtual ~CAkMusicPBI();

	
protected:
    AkUInt32			m_ulSourceOffset;//Offset to start from beginning, in Native samples, used for IM.
	AkUInt32			m_ulStopOffset;
    bool                m_bWasStoppedByUEngine;
};

#endif
