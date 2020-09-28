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

#include "ActionProxyLocal.h"
#include "AkActions.h"
#include "AkAudiolib.h"


ActionProxyLocal::ActionProxyLocal( AkActionType in_actionType, AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Action );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkAction::Create( in_actionType, in_id ) );
}

void ActionProxyLocal::SetElementID( AkUniqueID in_elementID )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		static_cast<CAkAction*>( GetIndexable() )->SetElementID( in_elementID );
	}
}

void ActionProxyLocal::SetActionType( AkActionType in_actionType )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		static_cast<CAkAction*>( GetIndexable() )->ActionType( in_actionType );
	}
}

void ActionProxyLocal::Delay( AkTimeMs in_delay, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		static_cast<CAkAction*>( GetIndexable() )->Delay( 
			CAkTimeConv::MillisecondsToSamples( in_delay ), 
			CAkTimeConv::MillisecondsToSamples( in_rangeMin ), 
			CAkTimeConv::MillisecondsToSamples( in_rangeMax ) );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionPlayProxyLocal::TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin, const AkTimeMs in_rangeMax )
{
	CAkActionPlay* pIndexable = static_cast<CAkActionPlay*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->TransitionTime( in_transitionTime, in_rangeMin, in_rangeMax );
	}
}

void ActionPlayProxyLocal::CurveType( const AkCurveInterpolation in_eCurveType )
{
	CAkActionPlay* pIndexable = static_cast<CAkActionPlay*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->CurveType( in_eCurveType );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionExceptProxyLocal::AddException( AkUniqueID in_elementID )
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->AddException( in_elementID );
	}
}

void ActionExceptProxyLocal::RemoveException( AkUniqueID in_elementID )
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveException( in_elementID );
	}
}

void ActionExceptProxyLocal::ClearExceptions()
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->ClearExceptions();
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionActiveProxyLocal::TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin, const AkTimeMs in_rangeMax )
{
	CAkActionActive* pIndexable = static_cast<CAkActionActive*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->TransitionTime( in_transitionTime, in_rangeMin, in_rangeMax );
	}
}

void ActionActiveProxyLocal::CurveType( const AkCurveInterpolation in_eCurveType )
{
	CAkActionActive* pIndexable = static_cast<CAkActionActive*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->CurveType( in_eCurveType );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionPauseProxyLocal::IncludePendingResume( bool in_bIncludePendingResume )
{
	CAkActionPause* pIndexable = static_cast<CAkActionPause*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IncludePendingResume( in_bIncludePendingResume );
	}
}
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionResumeProxyLocal::IsMasterResume( bool in_bIsMasterResume )
{
	CAkActionResume* pIndexable = static_cast<CAkActionResume*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IsMasterResume( in_bIsMasterResume );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetValueProxyLocal::TransitionTime( const AkTimeMs in_transitionTime, const AkTimeMs in_rangeMin , const AkTimeMs in_rangeMax )
{
	CAkActionSetValue* pIndexable = static_cast<CAkActionSetValue*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->TransitionTime( in_transitionTime, in_rangeMin, in_rangeMax );
	}
}

void ActionSetValueProxyLocal::CurveType( const AkCurveInterpolation in_eCurveType )
{
	CAkActionSetValue* pIndexable = static_cast<CAkActionSetValue*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->CurveType( in_eCurveType );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetPitchProxyLocal::SetValue( AkPitchValue in_pitchType, AkValueMeaning in_eValueMeaning, AkPitchValue in_rangeMin, AkPitchValue in_rangeMax )
{
	CAkActionSetPitch* pIndexable = static_cast<CAkActionSetPitch*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_pitchType, in_eValueMeaning, in_rangeMin, in_rangeMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetVolumeProxyLocal::SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkActionSetVolume* pIndexable = static_cast<CAkActionSetVolume*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_value, in_eValueMeaning, in_rangeMin, in_rangeMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetLFEProxyLocal::SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkActionSetLFE* pIndexable = static_cast<CAkActionSetLFE*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_value, in_eValueMeaning, in_rangeMin, in_rangeMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetLPFProxyLocal::SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkActionSetLPF* pIndexable = static_cast<CAkActionSetLPF*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_value, in_eValueMeaning, in_rangeMin, in_rangeMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetStateProxyLocal::SetGroup( AkStateGroupID in_groupID )
{
	CAkActionSetState* pIndexable = static_cast<CAkActionSetState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetStateGroup( in_groupID );
	}
}

void ActionSetStateProxyLocal::SetTargetState( AkStateID in_stateID )
{
	CAkActionSetState* pIndexable = static_cast<CAkActionSetState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetTargetState( in_stateID );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetSwitchProxyLocal::SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID )
{
	CAkActionSetSwitch* pIndexable = static_cast<CAkActionSetSwitch*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSwitchGroup( in_ulSwitchGroupID );
	}
}

void ActionSetSwitchProxyLocal::SetTargetSwitch( const AkSwitchStateID in_ulSwitchID )
{
	CAkActionSetSwitch* pIndexable = static_cast<CAkActionSetSwitch*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetTargetSwitch( in_ulSwitchID );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetRTPCProxyLocal::SetRTPCGroup( const AkRtpcID in_RTPCGroupID )
{
	CAkActionSetRTPC* pIndexable = static_cast<CAkActionSetRTPC*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetRTPCGroup( in_RTPCGroupID );
	}
}

void ActionSetRTPCProxyLocal::SetRTPCValue( const AkReal32 in_fRTPCValue )
{
	CAkActionSetRTPC* pIndexable = static_cast<CAkActionSetRTPC*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetRTPCValue( in_fRTPCValue );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionUseStateProxyLocal::UseState( bool in_bUseState )
{
	CAkActionUseState* pIndexable = static_cast<CAkActionUseState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->UseState( in_bUseState );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionBypassFXProxyLocal::BypassFX( bool in_bBypassFX )
{
	CAkActionBypassFX* pIndexable = static_cast<CAkActionBypassFX*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Bypass( in_bBypassFX );
	}
}

void ActionBypassFXProxyLocal::SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask )
{
	CAkActionBypassFX* pIndexable = static_cast<CAkActionBypassFX*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetBypassTarget( in_bBypassAllFX, in_ucEffectsMask );
	}
}

