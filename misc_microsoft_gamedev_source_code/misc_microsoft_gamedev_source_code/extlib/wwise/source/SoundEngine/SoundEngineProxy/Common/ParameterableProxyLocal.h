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

#include "AudioNodeProxyLocal.h"
#include "IParameterableProxy.h"

class CAkParameterNode;

class ParameterableProxyLocal : public AudioNodeProxyLocal
								, virtual public IParameterableProxy
{
public:
	// IParameterableProxy members
    virtual void Priority( AkPriority in_priority );
	virtual void PriorityApplyDistFactor( bool in_bApplyDistFactor );
	virtual void PriorityDistanceOffset( AkInt16 in_iDistOffset );
	virtual void PriorityOverrideParent( bool in_bOverrideParent );

	virtual void SetStateGroup( AkStateGroupID in_stateGroupID );
	virtual void UnsetStateGroup();
	virtual void AddState( AkUniqueID in_stateInstanceID, AkStateID in_stateID );
	virtual void RemoveState( AkStateID in_stateID );
	virtual void RemoveAllStates();
	virtual void UseState( bool in_bUseState );
	virtual void LinkStateToStateDefault( AkStateID in_stateID );
	virtual void SetStateSyncType( AkUInt32/*AkSyncType*/ in_eSyncType );

	virtual void SetFX( AkPluginID in_FXID,	AkUInt32 in_uFXIndex, void* in_pvInitParamsBlock = NULL, AkUInt32 in_ulParamBlockSize = 0);
	virtual void SetFXParam( AkPluginID in_FXID, AkUInt32 in_uFXIndex, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize );

	virtual void BypassAllFX( bool in_bBypass );
	virtual void BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass );

	virtual void RemoveFX( AkUInt32 in_uFXIndex );

	virtual void SetRTPC( AkPluginID in_FXID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion = NULL, AkUInt32 in_ulConversionArraySize = 0 );
	virtual void UnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );
};
