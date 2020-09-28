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
// FeedbackMixBus.h
//
//////////////////////////////////////////////////////////////////////

#include "AkVPLMixBusNode.h"

//On XBox, the AkMixer only accepts buffers that have a number of samples
//multiple of 16.  In feedback, we have only 8 samples per buffer.  Therefore
//we have to mix "manually".
class CAkFeedbackMixBus : public CAkVPLMixBusNode
{
public:
	AKRESULT ConsumeBuffer( AkVPLMixState & in_rMixState );
};

//Both pipelines use the same mixing object.
typedef CAkVPLMixBusNode CAkAudioToFeedbackMixBus;