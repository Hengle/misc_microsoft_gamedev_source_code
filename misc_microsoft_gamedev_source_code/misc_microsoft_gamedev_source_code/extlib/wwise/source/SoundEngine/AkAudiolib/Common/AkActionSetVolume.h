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
// AkActionSetVolume.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_SET_VOLUME_H_
#define _ACTION_SET_VOLUME_H_

#include "AkActionSetValue.h"
#include "AkParameters.h"

class CAkParameterNode;

class CAkActionSetVolume : public CAkActionSetValue
{
public:
	//Thread safe version of the constructor
	static CAkActionSetVolume* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	// Set the target value of the Action
	void SetValue(
		const AkReal32 in_fValue,// Target Value of the Action
		const AkValueMeaning in_eValueMeaning, //Target Meaning
		const AkReal32 in_RangeMin = 0,
		const AkReal32 in_RangeMax = 0
		);

protected:
	//Constructor
	CAkActionSetVolume(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionSetVolume();

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

	void ExecSetValueInternal(CAkParameterNodeBase* in_pNode, CAkRegisteredObj * in_pGameObj, AkValueMeaning in_eMeaning, AkReal32 in_fValue);

private:
	RANGED_PARAMETER<AkReal32> m_TargetValue;
	AkValueMeaning m_eValueMeaning;
};
#endif
