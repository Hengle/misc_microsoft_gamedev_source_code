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
// AkFeedbackMixBus.cpp
//
// XBox360 implementation.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "AkFeedbackMixBus.h"

//On XBox, the AkMixer only accepts buffers that have a number of samples
//multiple of 16.  In feedback, we have only 8 samples per buffer.  Therefore
//we have to mix "manually"
AKRESULT CAkFeedbackMixBus::ConsumeBuffer( AkVPLMixState & in_rMixState )
{
	//Interleave and mix. 
	AkAudioMix * pVolumes = in_rMixState.buffer.AudioMix;
	for(AkUInt32 iSource = 0; iSource < in_rMixState.buffer.NumChannels(); iSource++)
	{		
		for(AkUInt32 iDest = 0; iDest < m_BufferOut.NumChannels(); iDest++)
		{
			//Setup volumes for this channel
			//Warning: pointing directly to the floats in the structure.  If the structure 
			//changes, then the mix will change too.  But I don't think it will, because
			//the meaning of each volume is linked to the channel distribution on all platforms.
			AkReal32 *pNextDestVol = (AkReal32*)&pVolumes[iSource].Next;
			AkReal32 *pPrevDestVol = (AkReal32*)&pVolumes[iSource].Previous;
			AkReal32 fVolume = pPrevDestVol[iDest];
			AkReal32 fVolumeStep = (pNextDestVol[iDest] - fVolume) / in_rMixState.buffer.MaxFrames();

			//Setup sample pointers.
			AkReal32 *pResult = (AkReal32*)m_BufferOut.GetChannel(iDest);
			AkReal32 *pData = ((AkReal32*)in_rMixState.buffer.GetChannel(iSource));
			AkReal32 *pEnd = pData + in_rMixState.buffer.MaxFrames();
			while(pData < pEnd)
			{
				*pResult += *pData * fVolume;
				pResult++; 
				pData++;
				fVolume += fVolumeStep;
			}
		}

		pVolumes[iSource].Next.CopyTo(pVolumes[iSource].Previous.volumes);
	}

	m_BufferOut.uValidFrames = in_rMixState.buffer.MaxFrames();

	return AK_Success;
}
