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

#include "ParameterableProxyLocal.h"
#include "AkParameterNodeBase.h"
#include "AkCritical.h"


void ParameterableProxyLocal::Priority( AkPriority in_priority )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetPriority( in_priority );
	}
}

void ParameterableProxyLocal::PriorityApplyDistFactor( bool in_bApplyDistFactor )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetPriorityApplyDistFactor( in_bApplyDistFactor );
	}
}

void ParameterableProxyLocal::PriorityDistanceOffset( AkInt16 in_iDistOffset )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetPriorityDistanceOffset( (AkPriority) in_iDistOffset );
	}
}

void ParameterableProxyLocal::PriorityOverrideParent( bool in_bOverrideParent )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetPriorityOverrideParent( in_bOverrideParent );
	}
}

void ParameterableProxyLocal::SetStateGroup( AkStateGroupID in_stateGroupID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetStateGroup( in_stateGroupID );
	}
}

void ParameterableProxyLocal::UnsetStateGroup()
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->UnsetStateGroup();
	}
}

void ParameterableProxyLocal::AddState( AkUniqueID in_stateInstanceID, AkStateID in_stateID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->AddState( in_stateInstanceID, in_stateID );
	}
}

void ParameterableProxyLocal::RemoveState( AkStateID in_stateID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveState( in_stateID );
	}
}

void ParameterableProxyLocal::RemoveAllStates()
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveAllStates();
	}
}

void ParameterableProxyLocal::UseState( bool in_bUseState )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->UseState( in_bUseState );
	}
}

void ParameterableProxyLocal::LinkStateToStateDefault( AkStateID in_stateID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->LinkStateToStateDefault( in_stateID );
	}
}

void ParameterableProxyLocal::SetStateSyncType( AkUInt32/*AkSyncType*/ in_eSyncType )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetStateSyncType( in_eSyncType );
	}
}

void ParameterableProxyLocal::SetFX( AkPluginID in_FXID, AkUInt32 in_uFXIndex, void* in_pvInitParamsBlock /* = NULL */, AkUInt32 in_ulParamBlockSize /* = 0 */)
{
#ifdef RVL_OS
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // RVL_OS

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetFX( in_FXID, in_uFXIndex, in_pvInitParamsBlock, in_ulParamBlockSize );
	}
}

void ParameterableProxyLocal::SetFXParam( AkPluginID in_FXID, AkUInt32 in_uFXIndex, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize )
{
#ifdef RVL_OS
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // RVL_OS

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetFXParam( in_FXID, in_uFXIndex, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
}

void ParameterableProxyLocal::BypassAllFX( bool in_bBypass )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->MainBypassFX( ( in_bBypass ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG );
	}
}

void ParameterableProxyLocal::BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass )
{
#ifdef RVL_OS
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // RVL_OS

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->MainBypassFX( ( in_bBypass ? 1 : 0 ) << in_uFXIndex, 1 << in_uFXIndex );
	}
}

void ParameterableProxyLocal::RemoveFX( AkUInt32 in_uFXIndex )
{
#ifdef RVL_OS
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // RVL_OS

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveFX( in_uFXIndex );
	}
}

void ParameterableProxyLocal::SetRTPC( AkPluginID in_FXID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion /*= NULL*/, AkUInt32 in_ulConversionArraySize /*= 0*/ )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetRTPC( in_FXID, in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
}

void ParameterableProxyLocal::UnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->UnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );
	}
}
