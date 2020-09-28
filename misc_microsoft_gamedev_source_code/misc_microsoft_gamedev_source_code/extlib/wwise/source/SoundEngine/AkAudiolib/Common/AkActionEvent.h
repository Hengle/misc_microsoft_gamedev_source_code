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
// AkActionEvent.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_EVENT_H_
#define _ACTION_EVENT_H_

#include "AkAction.h"

class CAkActionEvent : public CAkAction
{

public:
	//Thread safe version of the constructor
	static CAkActionEvent* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);


	virtual void Delay(
		AkTimeMs in_Delay, // Custom ID
		AkTimeMs in_RangeMin = 0,// Range Min
		AkTimeMs in_RangeMax = 0// Range Max
		);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

	// Set the element ID associated to the Action
	// Overload to set the Event ID
	virtual void SetElementID(
		AkUniqueID in_ulElementID//Element ID set as action target
		);

protected:

	//Constructor
	CAkActionEvent(
		AkActionType in_eActionType,	//Type of action
		AkUniqueID in_ulID
		);

	//Destructor
	virtual ~CAkActionEvent();

	AKRESULT Init(){ return CAkAction::Init(); }

protected:

	AkUniqueID m_ulTargetEventID;	// Associated element
	

};
#endif
