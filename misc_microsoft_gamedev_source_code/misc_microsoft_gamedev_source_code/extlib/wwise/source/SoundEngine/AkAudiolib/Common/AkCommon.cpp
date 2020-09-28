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
// AkCommon.cpp
//
// Implementation of public AkAudioBuffer structure.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkCommon.h"

// AkPipelineBuffer data setters.
// ---------------------------------------

// Channel mask.
// Override SetChannelMask(): Constraint-free assignation.
AKRESULT AkPipelineBufferBase::SetChannelMask( AkChannelMask in_uChannelMask )
{
	uChannelMask = in_uChannelMask;
	return AK_Success;
}

#ifndef RVL_OS

// Deinterleaved data. 
// ---------------------------------------

// This implementation could be platform specific.
#include "AkLEngine.h"

AKRESULT AkPipelineBufferBase::GetCachedBuffer( 
	AkUInt16		in_uMaxFrames, 
	AkChannelMask	in_uChannelMask )	
{
	// Note. The current implementation uses one contiguous buffer.
	AkUInt32 uNumChannels = GetNumChannels( in_uChannelMask );
	AKASSERT( uNumChannels || !"Channel mask must be set before allocating audio buffer" );
	AkUInt32 uAllocSize = in_uMaxFrames * sizeof(AkReal32) * uNumChannels;
	void * pBuffer = CAkLEngine::GetCachedAudioBuffer( uAllocSize );
	if ( pBuffer )
	{
		pData = pBuffer;
		uMaxFrames = in_uMaxFrames;
		uChannelMask = in_uChannelMask;
		uValidFrames = 0;
		return AK_Success;
	}
	return AK_InsufficientMemory;
}
void AkPipelineBufferBase::ReleaseCachedBuffer()
{
	AKASSERT( pData && uMaxFrames > 0 && NumChannels() > 0 );
	AkUInt32 uAllocSize = uMaxFrames * sizeof(AkReal32) * NumChannels();
	CAkLEngine::ReleaseCachedAudioBuffer( uAllocSize, pData );
	pData = NULL;
	uMaxFrames = 0;
}
#endif // !RVL_OS
