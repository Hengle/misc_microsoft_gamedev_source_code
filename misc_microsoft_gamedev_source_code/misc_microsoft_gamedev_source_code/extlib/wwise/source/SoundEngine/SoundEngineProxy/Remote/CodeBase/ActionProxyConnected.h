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

#include "ObjectProxyConnected.h"
#include "ActionProxyLocal.h"

class ActionProxyConnected : public ObjectProxyConnected
{
public:
	ActionProxyConnected();
	virtual ~ActionProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() = 0;

	DECLARE_BASECLASS( ObjectProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPlayProxyConnected : public ActionProxyConnected
{
public:
	ActionPlayProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionPlayProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionPlayProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionExceptProxyConnected : public ActionProxyConnected
{
public:
	ActionExceptProxyConnected();
	virtual ~ActionExceptProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionActiveProxyConnected : public ActionExceptProxyConnected
{
public:
	ActionActiveProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionActiveProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionStopProxyConnected : public ActionActiveProxyConnected
{
public:
	ActionStopProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionStopProxyConnected();

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionActiveProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionActiveProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPauseProxyConnected : public ActionActiveProxyConnected
{
public:
	ActionPauseProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionPauseProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionPauseProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionActiveProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionResumeProxyConnected : public ActionActiveProxyConnected
{
public:
	ActionResumeProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionResumeProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionResumeProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionActiveProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBreakProxyConnected : public ActionProxyConnected
{
public:
	ActionBreakProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionBreakProxyConnected();

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionBreakProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetValueProxyConnected : public ActionExceptProxyConnected
{
public:
	ActionSetValueProxyConnected();
	virtual ~ActionSetValueProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	DECLARE_BASECLASS( ActionExceptProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionMuteProxyConnected : public ActionSetValueProxyConnected
{
public:
	ActionMuteProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionMuteProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionMuteProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionSetValueProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetPitchProxyConnected : public ActionSetValueProxyConnected
{
public:
	ActionSetPitchProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetPitchProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetPitchProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionSetValueProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetVolumeProxyConnected : public ActionSetValueProxyConnected
{
public:
	ActionSetVolumeProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetVolumeProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetVolumeProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionSetValueProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetLFEProxyConnected : public ActionSetValueProxyConnected
{
public:
	ActionSetLFEProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetLFEProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetLFEProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionSetValueProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetLPFProxyConnected : public ActionSetValueProxyConnected
{
public:
	ActionSetLPFProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetLPFProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetLPFProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionSetValueProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetStateProxyConnected : public ActionProxyConnected
{
public:
	ActionSetStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetStateProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetStateProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetSwitchProxyConnected : public ActionProxyConnected
{
public:
	ActionSetSwitchProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetSwitchProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetSwitchProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionTriggerProxyConnected : public ActionProxyConnected
{
public:
	ActionTriggerProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionTriggerProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionTriggerProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetRTPCProxyConnected : public ActionProxyConnected
{
public:
	ActionSetRTPCProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionSetRTPCProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionSetRTPCProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionUseStateProxyConnected : public ActionProxyConnected
{
public:
	ActionUseStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionUseStateProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionUseStateProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBypassFXProxyConnected : public ActionProxyConnected
{
public:
	ActionBypassFXProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionBypassFXProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionBypassFXProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};


// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionEventProxyConnected : public ActionProxyConnected
{
public:
	ActionEventProxyConnected( AkActionType in_actionType, AkUniqueID in_id );
	virtual ~ActionEventProxyConnected();

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer );

	virtual ActionProxyLocal & GetLocalProxy() { return m_proxyLocal; }

private:
	ActionEventProxyLocal m_proxyLocal;

	DECLARE_BASECLASS( ActionProxyConnected );
};
