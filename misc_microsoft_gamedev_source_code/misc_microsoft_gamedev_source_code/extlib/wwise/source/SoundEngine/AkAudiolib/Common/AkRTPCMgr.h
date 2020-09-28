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
// AkRTPCMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RTPC_MGR_H_
#define _RTPC_MGR_H_

#include "AkCommon.h"
#include "AkRTPC.h"
#include "AkHashList.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkConversionTable.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkList2.h"
#include "AkPoolSizes.h"
#include "AkMultiKeyList.h"

class CAkRegisteredObj;
class CAkSwitchAware;

typedef AkArray<CAkRegisteredObj *, CAkRegisteredObj *, ArrayPoolDefault, (DEFAULT_POOL_BLOCK_SIZE / sizeof(CAkRegisteredObj *))> GameObjExceptArray;

//Struct definitions
struct AkRTPCKey
{
	AkRtpcID m_RTPC_id;
	CAkRegisteredObj * m_pGameObj;
	bool operator ==(AkRTPCKey& in_Op)
	{
		return ( (m_RTPC_id == in_Op.m_RTPC_id) && (m_pGameObj == in_Op.m_pGameObj));
	}
};

struct AkSwitchKey
{
	AkSwitchGroupID m_SwitchGroup;
	CAkRegisteredObj * m_pGameObj;
	bool operator ==(AkSwitchKey& in_Op)
	{
		return ( (m_SwitchGroup == in_Op.m_SwitchGroup) && (m_pGameObj == in_Op.m_pGameObj) );
	}
};

// PhM : __w64 is compiler dependent + use the AkTypes
#if defined(WIN32) || defined(XBOX360)
inline AkUInt32 AkHash( AkRTPCKey in_key ) { return (AkUInt32) in_key.m_RTPC_id + (__w64 unsigned int) in_key.m_pGameObj; }
inline AkUInt32 AkHash( AkSwitchKey in_key ) { return (AkUInt32) in_key.m_SwitchGroup + (__w64 unsigned int) in_key.m_pGameObj; }
#else
inline AkUInt32 AkHash( AkRTPCKey in_key ) { return (AkUInt32) in_key.m_RTPC_id + (AkUInt32) in_key.m_pGameObj; }
inline AkUInt32 AkHash( AkSwitchKey in_key ) { return (AkUInt32) in_key.m_SwitchGroup + (AkUInt32) in_key.m_pGameObj; }
#endif
class CAkLayer;

// CAkRTPCMgr Class
// Unique per instance of Audiolib, the RTPC manager owns the real 
// time parameters and notified those who wants to be
class CAkRTPCMgr : public CAkObject
{	
public:
	enum SubscriberType
	{
		SubscriberType_IAkRTPCSubscriber	= 0,
		SubscriberType_CAkBus				= 1,
		SubscriberType_CAkParameterNodeBase	= 2,
		SubscriberType_CAkLayer				= 3,
		SubscriberType_PBI					= 4,
	};

friend class AkMonitor;

private:

	// A single RTPC Curve
	struct RTPCCurve
	{
		AkUniqueID RTPCCurveID;
		AkRtpcID RTPC_ID;
		CAkConversionTable<AkRTPCGraphPoint, AkReal32> ConversionTable;

		bool operator == ( const RTPCCurve& in_other )
		{
			return in_other.RTPCCurveID == RTPCCurveID;
		}
	};

	// Multiple RTPC Curves
	typedef AkArray<RTPCCurve, const RTPCCurve&, ArrayPoolDefault> RTPCCurveArray;

	// Each subscription, for a given Subscriber/Property/Game Object, now has
	// a list of Curves
	struct AkRTPCSubscription
	{
		void*				pSubscriber;	// Cast to appropriate interface/class depending on eType
		AkUInt32				ParamID;
		CAkRegisteredObj *	TargetGameObject;
		SubscriberType		eType;
		RTPCCurveArray		Curves;

		void Term()
		{
			for( RTPCCurveArray::Iterator iterCurves = Curves.Begin(); iterCurves != Curves.End(); ++iterCurves )
			{
				iterCurves.pItem->ConversionTable.Unset();
			}
			Curves.Term();
		}

		bool operator == ( const AkRTPCSubscription& in_other )
		{
			return in_other.pSubscriber == pSubscriber && in_other.ParamID == ParamID;
		}
	};

	struct AkSwitchSubscription
	{
		CAkSwitchAware*		pSwitch;
		AkSwitchGroupID		switchGroup;
	};

	typedef CAkList2<CAkSwitchAware*, CAkSwitchAware*, AkAllocAndKeep> AkListRTPCSwitchSubscribers;

	struct AkRTPCSwitchAssociation
	{
		AkSwitchGroupID											switchGroup;
		AkRtpcID												RTPC_ID;
		CAkConversionTable<AkRTPCGraphPointInteger, AkUInt32>	ConversionTable;
		AkListRTPCSwitchSubscribers listRTPCSwitchSubscribers;
	};

	// Helpers
	AKRESULT UpdateRTPCSubscriberInfo( void* in_pSubscriber );

	void UpdateSubscription( AkRTPCSubscription& in_rSubscription );


public:
	// Constructor
	CAkRTPCMgr();

	// Destructor
	virtual ~CAkRTPCMgr();

	AKRESULT Init();

	AKRESULT Term();

	///////////////////////////////////////////////////////////////////////////
	// Main SET func
	///////////////////////////////////////////////////////////////////////////

	AKRESULT AddSwitchRTPC(
		AkSwitchGroupID				in_switchGroup,
		AkRtpcID					in_RTPC_ID,
		AkRTPCGraphPointInteger*	in_pArrayConversion,//NULL if none
		AkUInt32						in_ulConversionArraySize//0 if none
		);

	void RemoveSwitchRTPC(
		AkSwitchGroupID				in_switchGroup
		);

	AKRESULT SetRTPCInternal( 
		AkRtpcID in_RTPCid, 
		AkReal32 in_Value, 
		CAkRegisteredObj * in_pGameObj = NULL
		);

	AKRESULT SetSwitchInternal( 
		AkSwitchGroupID in_SwitchGroup,
		AkSwitchStateID in_SwitchState,
		CAkRegisteredObj * in_pGameObj = NULL 
		);	

///////////////////////////////////////////////////////////////////////////
	// Main GET func
///////////////////////////////////////////////////////////////////////////

	void GetRTPCExceptions( 
		AkRtpcID in_RTPCid, 
		GameObjExceptArray& out_ExceptArrayObj 
		);

	AkSwitchStateID GetSwitch( 
		AkSwitchGroupID in_SwitchGroup, 
		CAkRegisteredObj * in_pGameObj = NULL 
		);

	AkReal32 GetRTPCConvertedValue(
		void*					in_pSubscriber,
		AkUInt32					in_ParamID,
		CAkRegisteredObj *		in_GameObj
		);

	AkReal32 GetRTPCConvertedValue( // IMPORTANT: must already hold the RTPCLock when calling this method
		void *					in_pToken,
		CAkRegisteredObj *		in_GameObj,
		AkRtpcID				in_RTPCid
		);

	bool GetRTPCValue(
		AkRtpcID in_RTPC_ID,
		CAkRegisteredObj* in_GameObj,
		AkReal32& out_value,
		bool& out_bGameObjectSpecificValue
		);

///////////////////////////////////////////////////////////////////////////
	// Subscription Functions
///////////////////////////////////////////////////////////////////////////

	AKRESULT SubscribeRTPC(
		void*						in_pSubscriber,
		AkRtpcID					in_RTPC_ID,
		AkUInt32						in_ParamID,		//# of the param that must be notified on change
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,//NULL if none
		AkUInt32						in_ulConversionArraySize,//0 if none
		CAkRegisteredObj *			in_TargetGameObject,
		SubscriberType				in_eType
		);

	AKRESULT SubscribeSwitch(
		CAkSwitchAware*		in_pSwitch,
		AkSwitchGroupID		in_switchGroup,
		bool				in_bForceNoRTPC = false
		);

	AKRESULT RegisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID );
	void UnregisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID );

	AKRESULT UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID, AkUniqueID in_RTPCCurveID, bool* out_bMoreCurvesRemaining = NULL );

	AKRESULT UnSubscribeRTPC( void* in_pSubscriber );

	void UnSubscribeSwitch( CAkSwitchAware* in_pSwitch );

	void UnregisterGameObject( CAkRegisteredObj * in_GameObj );

//	void Reset( CAkRegisteredObj * in_GameObj = NULL );

	void ResetSwitches( CAkRegisteredObj * in_GameObj = NULL );

	void ResetRTPC( CAkRegisteredObj * in_GameObj = NULL );

private:
	AkRTPCSubscription* FindSubscription( void* in_pSubscriber, AkUInt32 in_ParamID );

	typedef AkHashList<AkRTPCKey, AkReal32, 31> AkMapRTPCEntries;
	AkMapRTPCEntries m_RTPCEntries;

	typedef AkHashList<AkSwitchKey, AkSwitchStateID, 31> AkMapSwitchEntries;
	AkMapSwitchEntries m_SwitchEntries;

	typedef CAkList2<AkRTPCSubscription, const AkRTPCSubscription&, AkAllocAndKeep> AkListRTPCSubscribers;
	AkListRTPCSubscribers m_listRTPCSubscribers;

	typedef CAkList2<AkSwitchSubscription, const AkSwitchSubscription&, AkAllocAndKeep> AkListSwitchSubscribers;
	AkListSwitchSubscribers m_listSwitchSubscribers;

	typedef CAkList2<AkRTPCSwitchAssociation, const AkRTPCSwitchAssociation&, AkAllocAndKeep> AkListRTPCSwitch;
	AkListRTPCSwitch m_listRTPCSwitch;

	typedef CAkMultiKeyList<AkRtpcID, CAkLayer*, AkAllocAndKeep> AkMultimapRTPCLayerRegistration;
	AkMultimapRTPCLayerRegistration m_mapRTPCLayers;

	CAkLock m_RTPCLock;
};

extern CAkRTPCMgr* g_pRTPCMgr;

#endif //_RTPC_MGR_H_
