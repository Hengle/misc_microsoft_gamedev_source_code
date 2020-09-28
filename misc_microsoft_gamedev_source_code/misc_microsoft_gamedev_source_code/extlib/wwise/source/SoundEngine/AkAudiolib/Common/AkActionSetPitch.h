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
// AkActionSetPitch.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_PITCH_H_
#define _ACTION_SET_PITCH_H_

#include "AkActionSetValue.h"
#include "AkParameters.h"

class CAkActionSetPitch : public CAkActionSetValue
{
public:
	//Thread safe version of the constructor
	static CAkActionSetPitch* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	// Set the target value of the Action
	void SetValue(
		const AkPitchValue in_Value, //Target value of the action
		const AkValueMeaning in_eValueMeaning, //Meaning of the target value
		const AkPitchValue in_RangeMin = 0,
		const AkPitchValue in_RangeMax = 0
		);

protected:
	CAkActionSetPitch(AkActionType in_eActionType, AkUniqueID in_ulID);
	virtual ~CAkActionSetPitch();

	AKRESULT Init(){ return CAkActionSetValue::Init(); }

protected:

	// Sets the value on the Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	// Sets the value on the Object specified Member element
	virtual void ExecSetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Element
		);

	// Resets the value on the Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	// Resets the value on the Object specified Member element
	virtual void ExecResetValue(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Object
		);

	// Resets the value on the Member element of both global and object specific
	virtual void ExecResetValueAll(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode	// Target Parameter Node
		);

	virtual void ExecResetValueExcept(
		CAkParameterNodeBase* in_pNode,	// Target Parameter Node
		CAkRegisteredObj * in_pGameObj	// Target Game Object
		);

	virtual AKRESULT SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

private:
	RANGED_PARAMETER<AkPitchValue> m_TargetValue;
	AkValueMeaning m_eValueMeaning;
};
#endif
