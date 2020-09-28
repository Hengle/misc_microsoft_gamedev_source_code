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

#include <AK/Tools/Common/AkArray.h>

class CAkFeedbackBus;
class CAkSrcLpFilter;

#define AK_MAX_PLAYERS 4

class AkFeedbackParams
{

private:
	//Disable default constructor/destructor
	AkFeedbackParams(AkUInt16 in_iPlayers, AkUInt16 in_iChannels);
	~AkFeedbackParams();

	void Init(AkUInt16 in_iPlayers, AkUInt16 in_iChannels);

public:

	static AkFeedbackParams* Create(AkUInt16 in_iPlayers, AkUInt16 in_iChannels, AkPositioningType in_ePos);
	void Destroy();

	enum PrevNextVolume {PrevVolumes = 0, NextVolumes = 1};
	inline AkUInt32 VolumeIndex(PrevNextVolume in_iPrevNext, AkUInt32 in_iPlayer, AkUInt32 in_iChannels)
	{
		AkUInt32 uTableSize = m_uChannels * m_uPlayerCount;
		return in_iPrevNext * uTableSize + (in_iPlayer * m_uChannels + in_iChannels);
	}

	void StampOldVolumes();
	bool CreateLowPassFilter(AkChannelMask in_unChannelMask);

	CAkFeedbackBus * m_pOutput;		//The output the object is routed to.
	AkVolumeValue	m_NewVolume;	//The motion volume (including the bus)	(in DB)
	AkVolumeValue	m_AudioBusVolume; //The audio bus volumes to remove from the total.
	AkLPFType		m_LPF;			//The low pass value.
	AkPitchValue	m_MotionBusPitch;//Pitch of the motion Bus.
#ifndef RVL_OS	
	CAkSrcLpFilter * m_pLPFilter;	//Low pass filter memory for feedback audio
#endif	
	AkUInt16		m_usCompanyID;	//Device's company ID
	AkUInt16		m_usPluginID;	//Device's plugin ID
	AkReal32		m_fNextAttenuation[AK_MAX_PLAYERS];	//Attenuation to add on top of the position volumes.	(Linear)
	AkReal32		m_fNextHierarchyVol;//Volume for the hierarchy  (Linear)
	AkUInt16		m_uPlayerCount;	//Number of players (used for the volume array below)
	AkUInt16		m_uChannels;	//Number of channels of the associated source
	bool			m_bFirst;		//Used to know when to use new volumes as previous volumes (at the first buffer processed)

	static const AkUInt16 ALL_DEVICES = 0xFFFF;	//m_usPluginID will be set to this to indicate 
	//that the data should be sent to all devices
	static const AkUInt16 UNINITIALIZED = 0;	//m_usPluginID will be set to this to indicate 
	//that the structure is not initialized yet.

	//Volumes from the positioning algorithm.  This is a variable sized structure.  The number of volumes
	//is variable and allocated at the same time as the struct to avoid extra costly malloc/free calls.
	//KEEP AT THE END
	AkSIMDSpeakerVolumes m_Volumes[1];
};



