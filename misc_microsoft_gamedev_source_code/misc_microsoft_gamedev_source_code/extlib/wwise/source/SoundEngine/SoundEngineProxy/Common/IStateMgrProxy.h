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

class IStateMgrProxy
{
public:
	virtual void AddStateGroup( AkStateGroupID in_groupID ) const = 0;
	virtual void RemoveStateGroup( AkStateGroupID in_groupID ) const = 0;
	
	virtual void AddStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, AkTimeMs in_transitionTime, bool in_bIsShared ) const = 0;
	virtual void RemoveStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, bool in_bIsShared ) const = 0;
	virtual void ClearStateTransitions( AkStateGroupID in_groupID ) const = 0;
	virtual void SetDefaultTransitionTime( AkStateGroupID in_groupID, AkTimeMs in_transitionTime ) const = 0;

	virtual void AddState( AkStateGroupID in_groupID, AkStateID in_stateID, AkUniqueID in_stateUniqueID ) const = 0;
	virtual void RemoveState( AkStateGroupID in_groupID, AkStateID in_stateID ) const = 0;
	virtual void SetState( AkStateGroupID in_groupID, AkStateID in_stateID ) const = 0;
	virtual AkStateID GetState( AkStateGroupID in_groupID ) const = 0;

	enum MethodIDs
	{
		MethodAddStateGroup = 1,
		MethodRemoveStateGroup,
		MethodAddStateTransition,
		MethodRemoveStateTransition,
		MethodClearStateTransitions,
		MethodSetDefaultTransitionTime,
		MethodAddState,
		MethodRemoveState,
		MethodSetState,
		MethodGetState,
        
		LastMethodID
	};
};

