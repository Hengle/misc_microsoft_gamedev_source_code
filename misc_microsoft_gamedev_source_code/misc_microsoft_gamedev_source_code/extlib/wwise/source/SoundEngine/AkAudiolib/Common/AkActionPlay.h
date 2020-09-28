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
// AkActionPlay.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_PLAY_H_
#define _ACTION_PLAY_H_

#include "AkAction.h"

struct AkCntrHistArray;
class CAkTransition;

class CAkActionPlay : public CAkAction
{
public:
	//Thread safe version of the constructor
	static CAkActionPlay* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

protected:
	CAkActionPlay(AkActionType in_eActionType, AkUniqueID in_ulID);
	virtual ~CAkActionPlay();
	AKRESULT Init(){ return CAkAction::Init(); }

public:
	//Execute the action
	//Must be called only by the audiothread
	//
	//Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		);

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

	AkFileID GetFileID(){ return m_fileID; }

	void SetFileID( AkFileID in_fileID ){ m_fileID = in_fileID; }

	virtual void GetHistArray( AkCntrHistArray& out_rHistArray );

	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

private:

	RANGED_PARAMETER<AkTimeMs> m_TransitionTime;
	AkInt32		m_eFadeCurve		:AKCURVEINTERPOLATION_NUM_STORAGE_BIT;
	AkFileID	m_fileID;			// Used for the PrepareEvent mechanism, it tells from which bank to gather the hierarchy data.
};
#endif
