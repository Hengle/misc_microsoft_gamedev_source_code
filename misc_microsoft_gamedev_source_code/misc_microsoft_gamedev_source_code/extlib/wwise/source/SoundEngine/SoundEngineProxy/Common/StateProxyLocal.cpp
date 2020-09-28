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

#include "StateProxyLocal.h"

#include "AkState.h"
#include "AkAudiolib.h"
#include "AkCritical.h"


StateProxyLocal::StateProxyLocal( AkUniqueID in_id, bool in_bIsCustomState, AkStateGroupID in_StateGroupID )
{
	CAkIndexable* pIndexable;
	if( in_bIsCustomState )
		pIndexable = AK::SoundEngine::GetStateIndexable( in_id, AkIdxType_CustomState, in_StateGroupID );
	else
		pIndexable = AK::SoundEngine::GetStateIndexable( in_id, AkIdxType_State, in_StateGroupID );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkState::Create( in_id, in_bIsCustomState, in_StateGroupID ) );
}

StateProxyLocal::~StateProxyLocal()
{
}

void StateProxyLocal::Volume( AkReal32 in_volume )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Volume( in_volume );
	}
}

void StateProxyLocal::VolumeMeaning( AkValueMeaning in_eMeaning )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->VolumeMeaning( in_eMeaning );
	}
}

void StateProxyLocal::Pitch( AkPitchValue in_pitch )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Pitch( in_pitch );
	}
}

void StateProxyLocal::PitchMeaning( AkValueMeaning in_eMeaning )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->PitchMeaning( in_eMeaning );
	}
}

void StateProxyLocal::LPF( AkLPFType in_LPF )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LPF( in_LPF );
	}
}

void StateProxyLocal::LPFMeaning( AkValueMeaning in_eMeaning )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LPFMeaning( in_eMeaning );
	}
}

void StateProxyLocal::LFEVolume( AkReal32 in_LFEVolume )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LFEVolume( in_LFEVolume );
	}
}

void StateProxyLocal::LFEVolumeMeaning( AkValueMeaning in_eMeaning )
{
	CAkState* pIndexable = static_cast<CAkState*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->LFEVolumeMeaning( in_eMeaning );
	}
}
