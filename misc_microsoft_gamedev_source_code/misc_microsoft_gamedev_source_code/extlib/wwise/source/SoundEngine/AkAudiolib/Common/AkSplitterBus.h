
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
// AkSplitterBus.h
//
// The splitter bus acts as a proxy for all player busses.  
// It will redirect all calls to all attached busses.
// This is how multiplayer is handled.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IAkMotionMixBus.h"
#include "AkLEngineStructs.h"
#include "AkKeyArray.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkFeedbackStructs.h"
#include "AkFeedbackMixBus.h"

//////////////////////////////////////////////////////////////////////////
// CAkSplitterBus 
// This class represents one mixing bus in the upper engine.  However,
// in reality will hold all the player specific busses.  This enables a 
// different mix of sources for each players and devices.
//////////////////////////////////////////////////////////////////////////
class CAkSplitterBus 
{
public:
	CAkSplitterBus();
	~CAkSplitterBus();

	AKRESULT	AddBus(AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkChannelMask in_MixingFormat);
	AKRESULT	RemoveBus( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID );

	AKRESULT 	Term();

	void		GetBuffer(AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkAudioBufferFinalMix *&io_rAudioBuffer, AkAudioBufferFinalMix *&io_rFeedbackBuffer );
	void		ReleaseBuffers();
	void		MixAudioBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers );
	void		MixFeedbackBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers );

private:

	void GetVolumesForPlayer( AkUInt8 in_iPlayer, CAkPBI* in_pPBI, AkAudioMix* io_pMix, AkAudioMix* io_pOriginalMix, AkUInt32 in_uChannels );

	struct PlayerSlot
	{
		PlayerSlot();
		void Term();
		CAkAudioToFeedbackMixBus*	m_pAudioMixBus;
		CAkFeedbackMixBus*			m_pFeedbackMixBus;
		AkUInt32					m_DeviceID;
		AkChannelMask				m_MixingFormat;
	};

	AkArray<PlayerSlot, PlayerSlot&, ArrayPoolDefault> m_aBusses;
	AkUInt8					m_iMaxPlayers;
	AkUInt8					m_iMaxDevices;
};

