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

#include "ObjectProxyLocal.h"
#include "IActionProxy.h"

class CAkActionPlay;
class CAkActionActive;
class CAkActionSetValue;
class CAkActionMute;
class CAkActionSetPitch;
class CAkActionSetVolume;
class CAkActionSetLFE;
class CAkActionSetLPF;
class CAkActionSetState;
class CAkActionUseState;

class ActionProxyLocal : public ObjectProxyLocal
						, virtual public IActionProxy
{
public:
	// IActionProxy members
	virtual void SetElementID( AkUniqueID in_elementID );
	virtual void SetActionType( AkActionType );

	virtual void Delay( AkTimeMs in_delay, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax );

protected:
	ActionProxyLocal( AkActionType in_actionType, AkUniqueID in_id );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPlayProxyLocal : public ActionProxyLocal
						, virtual public IActionPlayProxy
{
public:
	ActionPlayProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin, const AkTimeMs in_rangeMax );
	virtual void CurveType( const AkCurveInterpolation in_eCurveType );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionExceptProxyLocal : public ActionProxyLocal
							, virtual public IActionExceptProxy
{
public:
	virtual void AddException( AkUniqueID in_elementID );
	virtual void RemoveException( AkUniqueID in_elementID );
	virtual void ClearExceptions();

protected:
	ActionExceptProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionActiveProxyLocal : public ActionExceptProxyLocal
								, virtual public IActionActiveProxy
{
public:
	ActionActiveProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin, const AkTimeMs in_rangeMax );
	virtual void CurveType( const AkCurveInterpolation in_eCurveType );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPauseProxyLocal : public ActionActiveProxyLocal
								, virtual public IActionPauseProxy
{
public:
	ActionPauseProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionActiveProxyLocal( in_actionType, in_id ){}

	virtual void IncludePendingResume( bool in_bIncludePendingResume );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionResumeProxyLocal : public ActionActiveProxyLocal
								, virtual public IActionResumeProxy
{
public:
	ActionResumeProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionActiveProxyLocal( in_actionType, in_id ){}

	virtual void IsMasterResume( bool in_bIsMasterResume );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBreakProxyLocal : public ActionProxyLocal
{
public:
	ActionBreakProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetValueProxyLocal : public ActionExceptProxyLocal
									, virtual public IActionSetValueProxy
{
public:
	virtual void TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin , const AkTimeMs in_rangeMax );
	virtual void CurveType( const AkCurveInterpolation in_eCurveType );

protected:
	ActionSetValueProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionMuteProxyLocal : public ActionSetValueProxyLocal
								, virtual public IActionMuteProxy
{
public:
	ActionMuteProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionSetValueProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetPitchProxyLocal : public ActionSetValueProxyLocal
								, virtual public IActionSetPitchProxy
{
public:
	ActionSetPitchProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionSetValueProxyLocal( in_actionType, in_id ){}

	virtual void SetValue( AkPitchValue in_pitchType, AkValueMeaning in_eValueMeaning, AkPitchValue in_rangeMin, AkPitchValue in_rangeMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetVolumeProxyLocal : public ActionSetValueProxyLocal
								, virtual public IActionSetVolumeProxy
{
public:
	ActionSetVolumeProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionSetValueProxyLocal( in_actionType, in_id ){}

	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetLFEProxyLocal : public ActionSetValueProxyLocal
								, virtual public IActionSetLFEProxy
{
public:
	ActionSetLFEProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionSetValueProxyLocal( in_actionType, in_id ){}

	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetLPFProxyLocal : public ActionSetValueProxyLocal
								, virtual public IActionSetLPFProxy
{
public:
	ActionSetLPFProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionSetValueProxyLocal( in_actionType, in_id ){}

	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetStateProxyLocal : public ActionProxyLocal
									, virtual public IActionSetStateProxy
{
public:
	ActionSetStateProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void SetGroup( AkStateGroupID in_groupID );
	virtual void SetTargetState( AkStateID in_stateID );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetSwitchProxyLocal : public ActionProxyLocal
									, virtual public IActionSetSwitchProxy
{
public:
	ActionSetSwitchProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID );
	virtual void SetTargetSwitch( const AkSwitchStateID in_ulSwitchID );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionTriggerProxyLocal : public ActionProxyLocal
									, virtual public IActionTriggerProxy
{
public:
	ActionTriggerProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetRTPCProxyLocal : public ActionProxyLocal
									, virtual public IActionSetRTPCProxy
{
public:
	ActionSetRTPCProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void SetRTPCGroup( const AkRtpcID in_RTPCGroupID );
	virtual void SetRTPCValue( const AkReal32 in_fRTPCValue );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionUseStateProxyLocal : public ActionProxyLocal
									, virtual public IActionUseStateProxy
{
public:
	ActionUseStateProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void UseState( bool in_bUseState );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBypassFXProxyLocal : public ActionExceptProxyLocal
									, virtual public IActionBypassFXProxy
{
public:
	ActionBypassFXProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void BypassFX( bool in_bBypassFX );
	virtual void SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionEventProxyLocal : public ActionProxyLocal
									, virtual public IActionEventProxy
{
public:
	ActionEventProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}
};
