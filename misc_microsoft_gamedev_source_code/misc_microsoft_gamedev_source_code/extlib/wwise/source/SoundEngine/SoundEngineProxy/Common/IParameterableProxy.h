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

#include "IAudioNodeProxy.h"

#include "AKParameters.h"
#include "AkRTPC.h"

class IParameterableProxy : virtual public IAudioNodeProxy
{
	DECLARE_BASECLASS( IAudioNodeProxy );
public:
    virtual void Priority( AkPriority in_priority ) = 0;
	virtual void PriorityApplyDistFactor( bool in_bApplyDistFactor ) = 0;
	virtual void PriorityDistanceOffset( AkInt16 in_iDistOffset ) = 0;
	virtual void PriorityOverrideParent( bool in_bOverrideParent ) = 0;

	virtual void SetStateGroup( AkStateGroupID in_stateGroupID ) = 0;
	virtual void UnsetStateGroup() = 0;
	virtual void AddState( AkUniqueID in_stateInstanceID, AkStateID in_stateID ) = 0;
	virtual void RemoveState( AkStateID in_stateID ) = 0;
	virtual void RemoveAllStates() = 0;
	virtual void UseState( bool in_bUseState ) = 0;
	virtual void LinkStateToStateDefault( AkStateID in_stateID ) = 0;
	virtual void SetStateSyncType( AkUInt32/*AkSyncType*/ in_eSyncType ) = 0;

	virtual void SetFX( AkPluginID in_FXID,	AkUInt32 in_uFXIndex, void* in_pvInitParamsBlock = NULL , AkUInt32 in_ulParamBlockSize = 0 ) = 0;

	virtual void SetFXParam( AkPluginID in_FXID, AkUInt32 in_uFXIndex, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize ) = 0;

	virtual void BypassAllFX( bool in_bBypass ) = 0;
	virtual void BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass ) = 0;

	virtual void RemoveFX( AkUInt32 in_uFXIndex ) = 0;

	virtual void SetRTPC( AkPluginID in_FXID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion = NULL, AkUInt32 in_ulConversionArraySize = 0 ) = 0;
	virtual void UnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID ) = 0;

	enum MethodIDs
	{
        MethodPriority = __base::LastMethodID,
		MethodPriorityApplyDistFactor,
		MethodPriorityDistanceOffset,
		MethodPriorityOverrideParent,
		MethodSetStateGroup,
		MethodUnsetStateGroup,
		MethodAddState,
		MethodRemoveState,
		MethodRemoveAllStates,
		MethodUseState,
		MethodLinkStateToStateDefault,
		MethodSetStateSyncType,

		MethodSetFX,
		MethodSetFXParam,
		MethodBypassAllFX,
		MethodBypassFX,
		MethodRemoveFX,

		MethodSetRTPC,
		MethodUnsetRTPC,

		LastMethodID
	};
};
