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

//////////////////////////////////////////////////////////////////////
//
// AkSoundBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKSOUND_BASE_H_
#define _AKSOUND_BASE_H_
#include "AkParameterNode.h"
#include "AkListBareLight.h"
#include "AkPBI.h"	//required to inline functions:  AddPBI() and RemovePBI()

class CAkSoundBase : public CAkParameterNode
{
public:

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj, AkUniqueID in_NodeID, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );

	virtual void ParamNotification( NotifParams& in_rParams );

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkUInt8			in_cMuteLevel,		// New muting level
		AkMutedMapItem& in_rMutedItem,		// Target Game Object
		bool			in_bIsFromBus = false
		);

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkUInt8 in_cMuteLevel,		   // New muting level
		CAkRegisteredObj *	in_pGameObj, //Target Game Object
		AkMutedMapItem& in_rMutedItem,  //Target Game Object
		bool in_bPrioritizeGameObjectSpecificItems
		);

	// Notify the children PBI that a change int the Positioning parameters occured from RTPC
	virtual void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// 
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a Positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		);

	// Notify the children PBI that a major change occured and that the 
	// Parameters must be recalculated
	virtual void RecalcNotification();

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_pGameObj,
		void* in_pExceptArray = NULL
		);

	virtual void NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	virtual void UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem );

#ifndef AK_OPTIMIZED
	virtual void UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32			in_ulParamBlockSize
		);

	virtual void InvalidatePaths();
#endif

	AkRTPCFXSubscriptionList* GetSourceRTPCSubscriptionList();

	//Set Sound Looping info
	void Loop(
		bool  in_bIsLoopEnabled,
		bool  in_bIsLoopInfinite,
		AkInt16 in_sLoopCount,
		AkInt16 in_sCountModMin = 0,
		AkInt16 in_sCountModMax = 0
		);
	
	AkInt16 Loop();

	void AddPBI( CAkPBI* in_pPBI )
	{
		m_listPBI.AddFirst( in_pPBI );
	}

	void RemovePBI( CAkPBI* in_pPBI )
	{
		m_listPBI.Remove( in_pPBI );
	}

	void MonitorNotif(AkMonitorData::NotificationReason in_eNotifReason, AkGameObjectID in_GameObjID, UserParams& in_rUserParams, PlayHistory& in_rPlayHistory);

protected:
	CAkSoundBase( AkUniqueID in_ulID );
	virtual ~CAkSoundBase();

	typedef AkListBareLight<CAkPBI> AkListLightCtxs;
	AkListLightCtxs m_listPBI;

	RANGED_MODIFIERS<AkInt16>	m_LoopMod;
	AkInt16						m_Loop;
};

#endif //_AKSOUND_BASE_H_
