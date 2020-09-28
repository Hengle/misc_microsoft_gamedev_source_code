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


#pragma once

#include "IContainerProxy.h"

#include "AkRanSeqCntr.h"

class CAkRanSeqCntr;

class IRanSeqContainerProxy : virtual public IContainerProxy
{
	DECLARE_BASECLASS( IContainerProxy );
public:
	virtual void Mode( AkContainerMode in_eMode	) = 0;
	virtual void IsGlobal( bool in_bIsGlobal ) = 0;

	// Sequence mode related methods
	virtual void AddPlaylistItem( AkUniqueID in_elementID, AkUInt8 in_weight = DEFAULT_RANDOM_WEIGHT ) = 0;
    virtual void RemovePlaylistItem( AkUniqueID in_elementID ) = 0;
	virtual void ClearPlaylist() = 0;
	virtual void ResetPlayListAtEachPlay( bool in_bResetPlayListAtEachPlay ) = 0;
	virtual void RestartBackward( bool in_bRestartBackward ) = 0;
	virtual void Continuous( bool in_bIsContinuous ) = 0;
	virtual void ForceNextToPlay( AkInt16 in_position, AkGameObjectID in_gameObjPtr = NULL, AkPlayingID in_playingID = NO_PLAYING_ID ) = 0;
	virtual AkInt16 NextToPlay( AkGameObjectID in_gameObjPtr = NULL ) = 0;

	// Random mode related methods
	virtual void RandomMode( AkRandomMode in_eRandomMode ) = 0;
	virtual void AvoidRepeatingCount( AkUInt16 in_count ) = 0;
	virtual void SetItemWeight( AkUniqueID in_itemID, AkUInt8 in_weight ) = 0;
	virtual void SetItemWeight( AkUInt16 in_position, AkUInt8 in_weight ) = 0;

	virtual void Loop( 
		bool in_bIsLoopEnabled, 
		bool in_bIsLoopInfinite, 
		AkInt16 in_loopCount) = 0;

	virtual void TransitionMode( AkTransitionMode in_eTransitionMode ) = 0;
	virtual void TransitionTime( AkTimeMs in_transitionTime, AkTimeMs in_rangeMin = 0, AkTimeMs in_rangeMax = 0 ) = 0;


	enum MethodIDs
	{
		MethodMode = __base::LastMethodID,
		MethodIsGlobal,
		MethodAddPlaylistItem,
		MethodRemovePlaylistItem,
		MethodClearPlaylist,
		MethodResetPlayListAtEachPlay,
		MethodRestartBackward,
		MethodContinuous,
		MethodForceNextToPlay,
		MethodNextToPlay,
		MethodRandomMode,
		MethodAvoidRepeatingCount,
		MethodSetItemWeight_withID,
		MethodSetItemWeight_withPosition,

		MethodLoop,
		MethodTransitionMode,
		MethodTransitionTime,

		LastMethodID
	};
};

