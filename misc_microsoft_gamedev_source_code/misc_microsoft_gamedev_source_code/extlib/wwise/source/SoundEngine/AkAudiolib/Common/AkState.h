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
// AkState.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _STATE_H_
#define _STATE_H_

#include "AkParameters.h"
#include "AkIndexable.h"

class CAkParameterNodeBase;

class CAkState :public CAkIndexable
{
	friend class CEventsUT;

public:

	//Thread safe version of the constructor
	static CAkState* Create( AkUniqueID in_ulID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID );

protected:
	//Constructor
	CAkState( AkUniqueID in_ulID, bool in_bIsCustomState, AkStateGroupID in_StateGroupID );

	//Destructor
	virtual ~CAkState();

	AKRESULT Init(){ return AddToIndex(); }

public:

	AKRESULT AddToIndex();
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// INTERNAL USE ONLY, not to be exported in the SDK
	// Gives the CAkState the opportunity to notify the parent it it was modified by the Wwise
	//
	// Return  - void - 
	void InitNotificationSystem(
		CAkParameterNodeBase * in_pNode //NULL means the parent is the StateMgr, otherwise it must be the parent node
		);

	// INTERNAL USE ONLY, not to be exported in the SDK
	// Gives the CAkState the opportunity to notify the parent it it was modified by the Wwise
	//
	// Return  - void - 
	void TermNotificationSystem();

	// Set the Volume of the state
	void Volume(
		AkVolumeValue in_Volume //Volume in dB
		);

	// Get the volume of the state
	//
	// Return - AkReal32 - Volume of the State
	AkReal32 Volume();

	// Set the meaning of the Volume
	void VolumeMeaning(
		AkValueMeaning in_eVolumeMeaning //Volume meaning
		);

	// Get the meaning of the Volume
	//
	// Return - AkValueMeaning - 
	AkValueMeaning VolumeMeaning();

	// Set the pitch of the state
	void Pitch(
		AkPitchValue in_Pitch //Pitch to set the state at
		);

	// Gets the Pitch of the state
	//
	// Return - AkReal32 - Pitch of the state
	AkPitchValue Pitch();

	// Set the meaning of the state Pitch
	void PitchMeaning(
		AkValueMeaning in_ePitchMeaning //Pitch Meaning
		);

	// Gets the Pitch Meaning
	//
	// Return - AkValueMeaning - State Pitch Meaning
	AkValueMeaning PitchMeaning();

	// Set the LPF of the state
	void LPF(
		AkLPFType in_LPF //LPF to set the state at
		);

	// Gets the LPF of the state
	//
	// Return - AkReal32 - LPF of the state
	AkLPFType LPF();

	// Set the meaning of the state Pitch
	void LPFMeaning(
		AkValueMeaning in_eLPFMeaning //LPF Meaning
		);

	// Gets the LPF Meaning
	//
	// Return - AkValueMeaning - State LPF Meaning
	AkValueMeaning LPFMeaning();

	//Sets the State LFE
	void LFEVolume(
		AkReal32 in_fLFE //State LFE
		);

	// Gets the State LFE
	//
	// Return - AkReal32 - State LFE
	AkReal32 LFEVolume();

	//Sets the LFE meaning
	void LFEVolumeMeaning(
		AkValueMeaning in_eLFEMeaning //State LFE meaning
		);

	// Get the State LFE MEaning
	//
	// Return - AkValueMeaning - LFE value meaning
	AkValueMeaning LFEVolumeMeaning();

	AKRESULT SetInitialValues(AkUInt8* pData, AkUInt32 ulDataSize);

private:

	void NotifyParent();
	
	AkStdParameters	m_Parameters;

	CAkParameterNodeBase* m_pParentForNotify;

	// bool bitfields
	AkUInt32 m_bNotifyStateMgr	: 1;
	AkUInt32 m_bIsCustomState	: 1;

	AkStateGroupID m_StateGroupID;

};

struct AkStateLink
{
	CAkState*	pState;
	AkUniqueID	ulStateID;
	AkUInt8		bUseStateGroupInfo :1;
};
#endif
