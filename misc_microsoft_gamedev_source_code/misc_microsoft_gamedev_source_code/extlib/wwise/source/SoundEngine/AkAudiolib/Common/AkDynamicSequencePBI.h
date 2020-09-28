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
// AkDynamicSequencePBI.h
//
//////////////////////////////////////////////////////////////////////
#ifndef AKDYNAMICSEQUENCEPBI_H
#define AKDYNAMICSEQUENCEPBI_H

#include "AkContinuousPBI.h"
#include <AK/SoundEngine/Common/AkDynamicSequence.h> // for DynamicSequenceType in the SDK

class CAkDynamicSequence;

class CAkDynamicSequencePBI : public CAkContinuousPBI
{
public:
	CAkDynamicSequencePBI(
		CAkSoundBase*	in_pSound,			// Sound associated to the PBI (NULL if none).	
		CAkSource*		in_pSource,
		CAkRegisteredObj * in_pGameObj,		// Game object.
		ContParams&		in_rCparameters,	// Continuation parameters.
		UserParams&		in_rUserParams,
		PlayHistory&	in_rPlayHistory,	// History stuff.
		bool			in_bIsFirst,		// Is it the first play of a series.
		AkUniqueID		in_SeqID,			// Sample accurate sequence id.	 
        CAkPBIAware*    in_pInstigator,
		AkPriority		in_Priority,
		AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType );

	virtual ~CAkDynamicSequencePBI();

	virtual void Term();

	// Get the information about the next sound to play in a continuous system
	virtual void PrepareNextToPlay( bool in_bIsPreliminary );

private:

	bool IsPlaybackCompleted();

	void PlayNextElement( AkUniqueID in_nextElementID, AkTimeMs in_delay );

	bool m_bRequestNextFromDynSeq;
	AK::SoundEngine::DynamicSequence::DynamicSequenceType m_eDynamicSequenceType;
};

#endif // AKDYNAMICSEQUENCEPBI_H
