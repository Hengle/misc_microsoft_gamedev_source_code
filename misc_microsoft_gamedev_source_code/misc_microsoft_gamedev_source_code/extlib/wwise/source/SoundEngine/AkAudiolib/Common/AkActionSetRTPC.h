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
// AkActionSetRTPC.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_RTPC_H_
#define _ACTION_SET_RTPC_H_

#include "AkAction.h"

class CAkActionSetRTPC : public CAkAction
{
public:
	// Thread safe version of the constructor
	static CAkActionSetRTPC* Create( AkActionType in_eActionType, AkUniqueID in_ulID = 0 );

	// Set the channel of the Action
	void SetRTPCGroup(
		const AkRtpcID in_RTPCGroupID // RTPC ID
		);

	// Set the Target switch of the Action
	void SetRTPCValue(
		const AkReal32 in_fRTPCValue // Target value
		);

	virtual AKRESULT SetActionParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	// Constructor
	CAkActionSetRTPC( AkActionType in_eActionType, AkUniqueID in_ulID );

	// Destructor
	virtual ~CAkActionSetRTPC();

	AKRESULT Init(){ return CAkAction::Init(); }
public:

	// Execute the Action
	// Must be called only by the audiothread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

private:
	AkRtpcID m_RTPC_ID; // RTPC ID
	AkReal32 m_fRTPCValue; // Target Value
};

#endif // _ACTION_SET_RTPC_H_
