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

#include "IObjectProxy.h"

#include "AkActions.h"

class IActionProxy : virtual public IObjectProxy
{
	DECLARE_BASECLASS( IObjectProxy );
public:
	virtual void SetElementID( AkUniqueID in_elementID ) = 0;
	virtual void SetActionType( AkActionType in_actionType ) = 0;

	virtual void Delay( AkTimeMs in_delay, AkTimeMs in_rangeMin = 0, AkTimeMs in_rangeMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetElementID = __base::LastMethodID,
		MethodSetActionType,
		MethodDelay,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionPlayProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin = 0, const AkTimeMs in_rangeMax = 0 ) = 0;
	virtual void CurveType( const AkCurveInterpolation in_eCurveType ) = 0;

	enum MethodIDs
	{
		MethodTransitionTime = __base::LastMethodID,
		MethodCurveType,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionExceptProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void AddException( AkUniqueID in_elementID ) = 0;
	virtual void RemoveException( AkUniqueID in_elementID ) = 0;
	virtual void ClearExceptions() = 0;

	enum MethodIDs
	{
		MethodAddException = __base::LastMethodID,
		MethodRemoveException,
		MethodClearExceptions,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionActiveProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin = 0, const AkTimeMs in_rangeMax = 0 ) = 0;
	virtual void CurveType( const AkCurveInterpolation in_eCurveType ) = 0;

	enum MethodIDs
	{
		MethodTransitionTime = __base::LastMethodID,
		MethodCurveType,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionPauseProxy : virtual public IActionActiveProxy
{
	DECLARE_BASECLASS( IActionActiveProxy );
public:
	virtual void IncludePendingResume( bool in_bIncludePendingResume ) = 0;

	enum MethodIDs
	{
		MethodIncludePendingResume = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionResumeProxy : virtual public IActionActiveProxy
{
	DECLARE_BASECLASS( IActionActiveProxy );
public:
	virtual void IsMasterResume( bool in_bIsMasterResume ) = 0;

	enum MethodIDs
	{
		MethodIsMasterResume = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetValueProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin = 0, const AkTimeMs in_rangeMax = 0 ) = 0;
	virtual void CurveType( const AkCurveInterpolation in_eCurveType ) = 0;

	enum MethodIDs
	{
		MethodTransitionTime = __base::LastMethodID,
		MethodCurveType,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionMuteProxy : virtual public IActionSetValueProxy
{
	DECLARE_BASECLASS( IActionSetValueProxy );
public:

	enum MethodIDs
	{
		LastMethodID = __base::LastMethodID
	};
};

class IActionSetPitchProxy : virtual public IActionSetValueProxy
{
	DECLARE_BASECLASS( IActionSetValueProxy );
public:
	virtual void SetValue( AkPitchValue in_pitchType, AkValueMeaning in_eValueMeaning, AkPitchValue in_rangeMin = 0, AkPitchValue in_rangeMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

class IActionSetVolumeProxy : virtual public IActionSetValueProxy
{
	DECLARE_BASECLASS( IActionSetValueProxy );
public:
	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin = 0, AkReal32 in_rangeMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

class IActionSetLFEProxy : virtual public IActionSetValueProxy
{
	DECLARE_BASECLASS( IActionSetValueProxy );
public:
	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin = 0, AkReal32 in_rangeMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

class IActionSetLPFProxy : virtual public IActionSetValueProxy
{
	DECLARE_BASECLASS( IActionSetValueProxy );
public:
	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin = 0, AkReal32 in_rangeMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetStateProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetGroup( AkStateGroupID in_groupID ) = 0;
	virtual void SetTargetState( AkStateID in_stateID ) = 0;

	enum MethodIDs
	{
		MethodSetGroup = __base::LastMethodID,
		MethodSetTargetState,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetSwitchProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID ) = 0;
	virtual void SetTargetSwitch( const AkSwitchStateID in_ulSwitchID ) = 0;

	enum MethodIDs
	{
		MethodSetSwitchGroup = __base::LastMethodID,
		MethodSetTargetSwitch,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionTriggerProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetRTPCProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetRTPCGroup( const AkRtpcID in_RTPCGroupID ) = 0;
	virtual void SetRTPCValue( const AkReal32 in_fRTPCValue ) = 0;

	enum MethodIDs
	{
		MethodSetRTPCGroup = __base::LastMethodID,
		MethodSetRTPCValue,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionUseStateProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void UseState( bool in_bUseState ) = 0;

	enum MethodIDs
	{
		MethodUseState = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionBypassFXProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void BypassFX( bool in_bBypassFX ) = 0;
	virtual void SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask ) = 0;

	enum MethodIDs
	{
		MethodBypassFX = __base::LastMethodID,
		MethodSetBypassTarget,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionEventProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:

};

