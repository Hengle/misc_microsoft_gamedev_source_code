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

#include "IStateMgrProxy.h"

class StateMgrProxyLocal : public IStateMgrProxy
{
public:
	StateMgrProxyLocal();
	virtual ~StateMgrProxyLocal();

	// IStateMgrProxy members
	virtual void AddStateGroup( AkStateGroupID in_groupID ) const;
	virtual void RemoveStateGroup( AkStateGroupID in_groupID ) const;
	
	virtual void AddStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, AkTimeMs in_transitionTime, bool in_bIsShared ) const;
	virtual void RemoveStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, bool in_bIsShared ) const;
	virtual void ClearStateTransitions( AkStateGroupID in_groupID ) const;
	virtual void SetDefaultTransitionTime( AkStateGroupID in_groupID, AkTimeMs in_transitionTime ) const;

	virtual void AddState( AkStateGroupID in_groupID, AkStateID in_stateID, AkUniqueID in_stateUniqueID ) const;
	virtual void RemoveState( AkStateGroupID in_groupID, AkStateID in_stateID ) const;
	virtual void SetState( AkStateGroupID in_groupID, AkStateID in_stateID ) const;
	virtual AkStateID GetState( AkStateGroupID in_groupID ) const;

private:
};
