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


#include "stdafx.h"

#include "StateMgrProxyLocal.h"

#include "AkAudiolib.h"
#include "AkStateMgr.h"
#include "AkCritical.h"


StateMgrProxyLocal::StateMgrProxyLocal()
{
}

StateMgrProxyLocal::~StateMgrProxyLocal()
{
}

void StateMgrProxyLocal::AddStateGroup( AkStateGroupID in_groupID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->AddStateGroup( in_groupID );
	}
}

void StateMgrProxyLocal::RemoveStateGroup( AkStateGroupID in_groupID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->RemoveStateGroup( in_groupID );
	}
}

void StateMgrProxyLocal::AddStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, AkTimeMs in_transitionTime, bool in_bIsShared ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->AddStateTransition( in_groupID, in_stateID1, in_stateID2, in_transitionTime, in_bIsShared );
	}
}

void StateMgrProxyLocal::RemoveStateTransition( AkStateGroupID in_groupID, AkStateID in_stateID1, AkStateID in_stateID2, bool in_bIsShared ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->RemoveStateTransition( in_groupID, in_stateID1, in_stateID2, in_bIsShared );
	}
}

void StateMgrProxyLocal::ClearStateTransitions( AkStateGroupID in_groupID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->ClearStateTransition( in_groupID );
	}
}

void StateMgrProxyLocal::SetDefaultTransitionTime( AkStateGroupID in_groupID, AkTimeMs in_transitionTime ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->SetdefaultTransitionTime( in_groupID, in_transitionTime );
	}
}

void StateMgrProxyLocal::AddState( AkStateGroupID in_groupID, AkStateID in_stateID, AkUniqueID in_stateUniqueID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->AddState( in_groupID, in_stateID, in_stateUniqueID );
	}
}

void StateMgrProxyLocal::RemoveState( AkStateGroupID in_groupID, AkStateID in_stateID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		g_pStateMgr->RemoveState( in_groupID, in_stateID );
	}
}

void StateMgrProxyLocal::SetState( AkStateGroupID in_groupID, AkStateID in_stateID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::SetState( in_groupID, in_stateID );
	}
}

AkStateID StateMgrProxyLocal::GetState( AkStateGroupID in_groupID ) const
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		return g_pStateMgr->GetState( in_groupID );
	}
	return 0;
}

