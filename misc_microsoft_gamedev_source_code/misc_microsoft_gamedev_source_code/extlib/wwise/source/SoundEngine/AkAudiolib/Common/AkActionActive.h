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
// AkActionActive.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_ACTIVE_H_
#define _ACTION_ACTIVE_H_

#include "AkAction.h"
#include "AkActionExcept.h"
#include "AkAudioNode.h"		// enum ActionParamType;

class CAkAudioNode;
class CAkPBI;

class CAkActionActive : public CAkActionExcept
{
public:

	//Sets the Transition time
	void TransitionTime(
		const AkTimeMs in_lTransitionTime, //Transition time
		const AkTimeMs in_RangeMin = 0,
		const AkTimeMs in_RangeMax = 0
		);

	// Get the Curve type of the transition
	AkForceInline AkCurveInterpolation CurveType() { return (AkCurveInterpolation) m_eFadeCurve; }

	// Sets the curve type of the transition
	AkForceInline void CurveType(
		const AkCurveInterpolation in_eCurveType //Curve type
		)
	{
		m_eFadeCurve = in_eCurveType;
	}

protected:

	//Constructor
	CAkActionActive(AkActionType in_eActionType, AkUniqueID in_ulID);

	//Destructor
	virtual ~CAkActionActive();

	AKRESULT Init(){ return CAkActionExcept::Init(); }


protected:
	//Execute object specific execution
	AKRESULT Exec(
		ActionParamType in_eType,
		CAkRegisteredObj * in_pGameObj = NULL		//Game object pointer
		);

	void AllExec(
		ActionParamType in_eType,
		CAkRegisteredObj * in_pGameObj = NULL
		);

	void AllExecExcept(
		ActionParamType in_eType,
		CAkRegisteredObj * in_pGameObj = NULL
		);

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:
	RANGED_PARAMETER<AkTimeMs> m_TransitionTime;
	AkCurveInterpolation	m_eFadeCurve		:AKCURVEINTERPOLATION_NUM_STORAGE_BIT;
	bool		m_bIsMasterResume;
};
#endif
