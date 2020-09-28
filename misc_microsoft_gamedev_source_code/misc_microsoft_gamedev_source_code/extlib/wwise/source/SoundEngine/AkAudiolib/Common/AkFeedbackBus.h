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
// AkFeedbackBus.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _FEEDBACK_BUS_H_
#define _FEEDBACK_BUS_H_

#include "AkBus.h"

class CAkFeedbackBus : public CAkBus
{
public:
	
	//Thread safe version of the constructor
	static CAkFeedbackBus* Create(AkUniqueID in_ulID = 0);

	virtual AkNodeCategory NodeCategory();	

	virtual AKRESULT AddChild(
        AkUniqueID in_ulID          // Input node ID to add
		);

	virtual AKRESULT RemoveChild(
        AkUniqueID in_ulID          // Input node ID to remove
		);
	virtual AKRESULT CanAddChild( 
		CAkAudioNode * in_pAudioNode 
		);

	virtual void ParamNotification( NotifParams& in_rParams );

	static CAkFeedbackBus* GetMasterBus();

	// Manage the master bus when loading
	static void ResetMasterBus(CAkFeedbackBus* in_pBus);
	static CAkFeedbackBus* ClearTempMasterBus();

	// Get the compounded feedback parameters.  There is currenly only the volume.
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetFeedbackParameters( 
		AkFeedbackParams &io_Params,			// Parameters
		CAkSource *in_pSource,
		CAkRegisteredObj * in_GameObjPtr,		// Game object associated to the query
		bool in_bDoBusCheck = true );

	// Notify the Children PBIs that a (feedback) VOLUME variation occured.
	virtual void VolumeNotification(
		AkVolumeValue in_Volume,				// Volume variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a (feedback bus) Pitch variation occured.
	virtual void PitchNotification(
		AkPitchValue in_Pitch,					// Pitch variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

private:
	CAkFeedbackBus(AkUniqueID in_ulID);
	~CAkFeedbackBus();
	
	//Master bus management
	static CAkFeedbackBus* s_pMasterBus;
};

#endif
