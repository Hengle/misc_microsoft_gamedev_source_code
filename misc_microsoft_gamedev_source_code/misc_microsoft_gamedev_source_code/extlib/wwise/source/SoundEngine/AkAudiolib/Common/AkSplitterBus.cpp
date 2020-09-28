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

#include "stdafx.h"
#include "AkSplitterBus.h"
#include "AkSink.h"
#include "AkVPLMixBusNode.h"
#include "AkFeedbackMgr.h"
#include "AkLEngine.h"
#include <AK\Tools\Common\AkPlatformFuncs.h>
#include <AK\SoundEngine\Common\AkCommonDefs.h>

CAkSplitterBus::PlayerSlot::PlayerSlot()
{
	m_pFeedbackMixBus = NULL; 
	m_pAudioMixBus = NULL; 
	m_DeviceID = 0;
}

void CAkSplitterBus::PlayerSlot::Term()
{
	if (m_pFeedbackMixBus != NULL)
	{
#ifndef RVL_OS
		m_pFeedbackMixBus->Disconnect();
#endif
		AKVERIFY(m_pFeedbackMixBus->Term());	
		AkDelete2(g_LEngineDefaultPoolId, CAkFeedbackMixBus, m_pFeedbackMixBus);
		m_pFeedbackMixBus = NULL;
	}

	if (m_pAudioMixBus != NULL)
	{
#ifndef RVL_OS
		m_pAudioMixBus->Disconnect();
#endif
		AKVERIFY(m_pAudioMixBus->Term());
		AkDelete2(g_LEngineDefaultPoolId, CAkAudioToFeedbackMixBus, m_pAudioMixBus);
		m_pAudioMixBus = NULL;
	}
};

CAkSplitterBus::CAkSplitterBus()
{
	m_iMaxPlayers = 0;
	m_iMaxDevices = 0;
}

CAkSplitterBus::~CAkSplitterBus()
{	
	Term();
}

AKRESULT CAkSplitterBus::Term()
{	
	for(AkUInt32 i = 0; i < m_aBusses.Length(); i++)
	{
		m_aBusses[i].Term();
	}
	m_aBusses.Term();
	return AK_Success;
}

AKRESULT CAkSplitterBus::AddBus( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkChannelMask in_MixingFormat )
{
	//Check if the player is known
	bool bNeedNewPlayer = in_iPlayerID >= m_iMaxPlayers;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	bool bNeedNewDevice = iDevice == m_iMaxDevices;
	if (bNeedNewPlayer || bNeedNewDevice)
	{
		AkInt32 iOldPlayers = (AkInt32)m_iMaxPlayers;
		AkInt32 iOldDevices = (AkInt32)m_iMaxDevices;

		m_iMaxPlayers = AkMax(m_iMaxPlayers, in_iPlayerID + 1);
		if (bNeedNewDevice)
			m_iMaxDevices++; 

		if (!m_aBusses.Resize(m_iMaxPlayers * m_iMaxDevices))
			return AK_Fail;	//No more memory;

		if (bNeedNewDevice)
		{
			//Copy the data at the right places.  The array is basically a 2D array with the Player
			//number as the Y value and the device as the X.  So the layout looks like this
			// P0D0, P0D1, P1D0, P1D1 .... (where PyDx is player Y-device X)
			//Now that we want to add a device or a player we need to space the actual data differently.
			AkInt32 i = iOldPlayers - 1;
			for(; i >= 0; i--)
			{
				memcpy(&m_aBusses[i * m_iMaxDevices], &m_aBusses[i * iOldDevices], sizeof(PlayerSlot) * iOldDevices);
			}

			//Put the new device ID in the last slot for all players
			for(i = 0; i < m_iMaxPlayers; i++)
			{
				m_aBusses[i*m_iMaxDevices + (m_iMaxDevices-1)].m_DeviceID = in_iDeviceID;
			}
		}

		if (bNeedNewPlayer)
		{
			//Init the new player devices like the ones we know
			AkUInt32 iLast = (m_iMaxPlayers - 1) * m_iMaxDevices;
			for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
				m_aBusses[iLast + iDevice].m_DeviceID = m_aBusses[iDevice].m_DeviceID;
		}
	}

	//Once we make it here, we know there is a slot for this combination of player/device
	PlayerSlot &rSlot = m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice];
	if (rSlot.m_pFeedbackMixBus != NULL)
	{
		return AK_Success;	//Already initialized
	}

	//First initialization
	CAkFeedbackMixBus* pMixBus = NULL;
	AkNew2( pMixBus, g_LEngineDefaultPoolId, CAkFeedbackMixBus, CAkFeedbackMixBus() );
	if (pMixBus == NULL)
		return AK_Fail;

	rSlot.m_MixingFormat = in_MixingFormat;
	AKRESULT akr = pMixBus->Init(in_MixingFormat, AK_FEEDBACK_MAX_FRAMES_PER_BUFFER);
	if (akr != AK_Success)
	{
		AKVERIFY(pMixBus->Term());
		AkDelete2(g_LEngineDefaultPoolId, CAkFeedbackMixBus, pMixBus);
		return akr;
	}

#ifndef RVL_OS
	pMixBus->Connect();
#endif

	rSlot.m_pFeedbackMixBus = pMixBus;

	return AK_Success;
}

AKRESULT CAkSplitterBus::RemoveBus( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID )
{
	//Here we don't remove anything from the array.  We just uninitialize.
	if (in_iPlayerID >= m_iMaxPlayers)
		return AK_Fail;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	if (iDevice == m_iMaxDevices)
		return AK_Fail;
	
	m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice].Term();

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////////
// MixFeedbackBuffer: Adds a feedback source to the mix
// Params:
//	AkAudioBufferMix *io_pBuffer : VPL representing the source
///////////////////////////////////////////////////////////////////////////////
void CAkSplitterBus::MixFeedbackBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers )
{
	AkUInt32 iSlot = 0;
	CAkPBI *pPBI = io_runningVPL.pSrc->m_Src.GetContext();
	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();
	AkUInt32 uChannels = AK::GetNumChannels( io_runningVPL.state.buffer.GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY );
	AkPositioningType ePosType = pPBI->GetPositioningType();

	AkVPLMixState stateMix;
	stateMix.buffer = io_runningVPL.state.buffer;

	//Only "3D game defined" has different volumes for each player. 
	//If not in 3D, assume player 0 and compute them only once.
	if (ePosType != Ak3DGameDef)
		GetVolumesForPlayer(0, pPBI, stateMix.buffer.AudioMix, io_runningVPL.state.buffer.AudioMix, uChannels);	

	AkUInt32 keyDevice = MAKE_DEVICE_KEY(pParams->m_usCompanyID, pParams->m_usPluginID);
	for(AkUInt8 iPlayer = 0; iPlayer < m_iMaxPlayers; iPlayer++)
	{
		//Check if this player feels this source.
		if ((in_uPlayers & (1 << iPlayer)) == 0)
		{
			iSlot += m_iMaxDevices;
			continue;
		}

		//Change the volumes based on the target player (if 3D) and apply the master player volume.  
		if (ePosType == Ak3DGameDef)
			GetVolumesForPlayer(iPlayer, pPBI, stateMix.buffer.AudioMix, io_runningVPL.state.buffer.AudioMix, uChannels);

		for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
		{
			PlayerSlot& rSlot = m_aBusses[iPlayer * m_iMaxDevices + iDevice];

			if (rSlot.m_DeviceID == keyDevice)
			{
				if (rSlot.m_pFeedbackMixBus != NULL)
					rSlot.m_pFeedbackMixBus->ConsumeBuffer(stateMix);
				break;	//Found it.
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// MixAudioBuffer: Adds an audio source to the audio mix to be converted to
//				   vibrations.
// Params:
//	AkAudioBufferMix &io_rBuffer : the data and its volume
///////////////////////////////////////////////////////////////////////////////
void CAkSplitterBus::MixAudioBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers )
{
#ifndef RVL_OS
	AkVPLMixState *pState;
	CAkPBI *pPBI = io_runningVPL.pSrc->m_Src.GetContext();
	AkPositioningType ePosType = pPBI->GetPositioningType();
	AkUInt32 uChannels = AK::GetNumChannels( io_runningVPL.state.buffer.GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY );

#ifndef AK_PS3
	AkVPLMixState stateTemp;

	//If we processed the LPF, use that audio buffer.
	if (io_runningVPL.pFeedbackData->LPFBuffer.HasData())
		stateTemp.buffer.AttachContiguousDeinterleavedData(io_runningVPL.pFeedbackData->LPFBuffer.GetContiguousDeinterleavedData(), 
															io_runningVPL.pFeedbackData->LPFBuffer.MaxFrames(),
															io_runningVPL.pFeedbackData->LPFBuffer.uValidFrames,
															io_runningVPL.pFeedbackData->LPFBuffer.GetChannelMask());
	else
		stateTemp.buffer.AttachContiguousDeinterleavedData( io_runningVPL.state.buffer.GetContiguousDeinterleavedData(), 
															io_runningVPL.state.buffer.MaxFrames(),
															io_runningVPL.state.buffer.uValidFrames,
															io_runningVPL.state.buffer.GetChannelMask());
	pState = &stateTemp;
	//For PS3, the buffer was already copied in PrepareAudioProcessing.

	//Only "3D game defined" has different volumes for each player. 
	//If not in 3D, assume player 0 and compute them only once.
	if (ePosType != Ak3DGameDef)
		GetVolumesForPlayer(0, pPBI, pState->buffer.AudioMix, io_runningVPL.state.buffer.AudioMix, uChannels);	
#endif

	AkUInt32 iSlot = 0;
	for(AkUInt8 iPlayer = 0; iPlayer < m_iMaxPlayers; iPlayer++)
	{
		if ((in_uPlayers & (1 << iPlayer)) == 0)
		{
			iSlot += m_iMaxDevices;	//Skip this player entirely
			continue;
		}

#ifdef AK_PS3
		//On the PS3, each player has its own buffer for mixing.
		pState = &io_runningVPL.pFeedbackData->pStates[iPlayer];
#else
		//Change the volumes based on the target player (if 3D) and apply the master player volume.  (Except on PS3 since we need a set of volumes for each).
		if (ePosType == Ak3DGameDef)
#endif
			GetVolumesForPlayer(iPlayer, pPBI, pState->buffer.AudioMix, io_runningVPL.state.buffer.AudioMix, uChannels);

		for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
		{
			PlayerSlot& rSlot = m_aBusses[iSlot];

			//Check if the audio mix node exist, if not create only if this slot is active.
			if (rSlot.m_pFeedbackMixBus != NULL)
			{
				if(rSlot.m_pAudioMixBus == NULL)
				{
					AkNew2( rSlot.m_pAudioMixBus, g_LEngineDefaultPoolId, CAkAudioToFeedbackMixBus, CAkAudioToFeedbackMixBus() );
					if (rSlot.m_pAudioMixBus == NULL)
						continue ;		//Don't care about this failure.

					if(rSlot.m_pAudioMixBus->Init(rSlot.m_MixingFormat, LE_MAX_FRAMES_PER_BUFFER) != AK_Success)
					{
						AkDelete2(g_LEngineDefaultPoolId, CAkAudioToFeedbackMixBus, rSlot.m_pAudioMixBus);
						rSlot.m_pAudioMixBus = NULL;
						continue;
					}
					rSlot.m_pAudioMixBus->Connect();
				}
				rSlot.m_pAudioMixBus->ConsumeBuffer(*pState);
			}
			iSlot++;
		}
	}
#endif
}


void CAkSplitterBus::GetBuffer( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkAudioBufferFinalMix *& out_pAudioBuffer, AkAudioBufferFinalMix *& out_pFeedbackBuffer )
{
	out_pAudioBuffer = NULL;
	out_pFeedbackBuffer = NULL;

	if (in_iPlayerID >= m_iMaxPlayers)
		return;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	if (iDevice == m_iMaxDevices)
		return;

	PlayerSlot & rSlot = m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice];
	if (rSlot.m_pAudioMixBus != NULL)
		rSlot.m_pAudioMixBus->GetResultingBuffer(out_pAudioBuffer);

	if (rSlot.m_pFeedbackMixBus != NULL)
		rSlot.m_pFeedbackMixBus->GetResultingBuffer(out_pFeedbackBuffer);
}

void CAkSplitterBus::ReleaseBuffers()
{
	for(AkUInt32 i = 0; i < m_aBusses.Length(); ++i)
	{
		PlayerSlot& rSlot = m_aBusses[i];

		if (rSlot.m_pAudioMixBus != NULL)
			rSlot.m_pAudioMixBus->ReleaseBuffer();

		if (rSlot.m_pFeedbackMixBus != NULL)
			rSlot.m_pFeedbackMixBus->ReleaseBuffer();
	}
}

static AkReal32 ComputeNormalizationFactor(AkSIMDSpeakerVolumes* in_pVolumes, AkUInt32 in_iChannels)
{
	//Boost volume to ensure full displacement for motion sources.
	//We simply normalize to ensure that once mixed, the loudest channel is a 0dB.
	AkReal32 fMaxVol = 0;
	AkSIMDSpeakerVolumes volSum;
	volSum = in_pVolumes[0];
	for (AkUInt32 uChan=1; uChan < in_iChannels; uChan++ )
		volSum.Add(in_pVolumes[uChan]);

	fMaxVol = AkMax(volSum.volumes.fFrontLeft, volSum.volumes.fFrontRight);
	fMaxVol = AkMax(fMaxVol, volSum.volumes.fRearLeft);
	fMaxVol = AkMax(fMaxVol, volSum.volumes.fRearRight);

	return 1.0f/fMaxVol;
}

static AkReal32 dBToLinWithPositive(AkReal32 in_fVal)
{
	AkReal32 fLinVal = 0;
	if (in_fVal > 0)
	{
		fLinVal = AkMath::dBToLin(-in_fVal);
		if (fLinVal > 0)
			fLinVal = 1/fLinVal;
		else
			fLinVal = 65535;	//Infinite
	}
	else
		fLinVal = AkMath::dBToLin(in_fVal);

	return fLinVal;
}

void CAkSplitterBus::GetVolumesForPlayer( AkUInt8 in_iPlayer, CAkPBI* in_pPBI, AkAudioMix* io_pMix, AkAudioMix* io_pOriginalMix, AkUInt32 in_uChannels )
{
	AkPositioningType ePosType = in_pPBI->GetPositioningType();
	AkFeedbackParams *pParams = in_pPBI->GetFeedbackParameters();

	//Compute feedback offset 
	AkReal32 fMotionVol = dBToLinWithPositive(pParams->m_NewVolume);
	AkUInt32 iIndex;

	if (ePosType == Ak3DGameDef)
	{
		//In 3D game defined, each player may be in a different position.  Therefore, the volumes
		//from the positioning computations will be different.  The ones received in the AudioMix
		//array are the ones computed as if the audio was the output (taking the max value).  Discard those.
		pParams->m_fNextHierarchyVol = AkMath::dBToLin(in_pPBI->GetVolume() - pParams->m_AudioBusVolume);
		iIndex = pParams->VolumeIndex(AkFeedbackParams::NextVolumes, in_iPlayer, 0);
		for(AkUInt32 iChannel = 0; iChannel < in_uChannels; iChannel++)
		{
			io_pMix[iChannel].Next = pParams->m_Volumes[iIndex];
			iIndex++;
		}
	}
	else
	{
		//We must reset the volumes to their original values if there is more than one player.
		//If not, some volumes might be compounded (Motion Bus volume, Player master volume, audio bus reduction).
		//On top of that, the volumes for the audio pipeline must not be touched, so we need to work on a copy.
		for(AkUInt32 iChannel = 0; iChannel < in_uChannels; iChannel++)
			io_pMix[iChannel].Next = io_pOriginalMix[iChannel].Next;

		//Hierarchy volume is already accounted for.  But we must remove the audio bus volumes.
		pParams->m_fNextHierarchyVol = dBToLinWithPositive(-pParams->m_AudioBusVolume);
	}
		
	//Normalize the positioning volumes because the computations are done with constant power maths.
	//For feedback, we want to keep the maximum amplitude constant before and after positioning.
	iIndex = pParams->VolumeIndex(AkFeedbackParams::NextVolumes, in_iPlayer, 0);
	AkReal32 fNormFactor = ComputeNormalizationFactor(&pParams->m_Volumes[iIndex], in_uChannels);

	//Compute the next volume
	AkReal32 fNewVol = fNormFactor * 
						fMotionVol * 
						pParams->m_fNextAttenuation[in_iPlayer] * 
						pParams->m_fNextHierarchyVol;

	iIndex = pParams->VolumeIndex(AkFeedbackParams::PrevVolumes, in_iPlayer, 0);
	for(AkUInt8 iChannel = 0; iChannel < in_uChannels; iChannel++)
	{
		io_pMix[iChannel].Next.Mul(fNewVol);

		if (pParams->m_bFirst)
			io_pMix[iChannel].Previous = io_pMix[iChannel].Next;
		else
			io_pMix[iChannel].Previous = pParams->m_Volumes[iIndex++];		
	}

	//We used the new volumes, now they are old.
	pParams->StampOldVolumes();
}

