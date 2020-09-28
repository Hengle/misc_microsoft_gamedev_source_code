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
// AkRTPCMgr.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkRTPCMgr.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkSwitchCntr.h"
#include "AkDefault3DParams.h"
#include "AkMonitor.h"
#include "AkEvent.h"
#include "AkActionSetSwitch.h"
#include "AkAudioLib.h"
#include "AkRegisteredObj.h"
#include "AkLayer.h"
#include "AkPBI.h"

#define MIN_SIZE_ENTRIES 64
#define MIN_SIZE_ENTRIES_LOW 16
#define MAX_SIZE_ENTRIES AK_NO_MAX_LIST_SIZE

#define DEFAULT_RTPC_VALUE 0
#define DEFAULT_SWITCH_TYPE 0

// PhM : __w64 is compiler dependent + use the AkTypes
#if defined(WIN32) || defined(XBOX360)
inline AkUInt32 AkHash( CAkRegisteredObj *  in_obj ) { return (AkUInt32) (__w64 AkUInt32) in_obj; }
#else
inline AkUInt32 AkHash( CAkRegisteredObj *  in_obj ) { return (AkUInt32) in_obj; }
#endif

CAkRTPCMgr::CAkRTPCMgr()
{
}

CAkRTPCMgr::~CAkRTPCMgr()
{
}

AKRESULT CAkRTPCMgr::Init()
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKRESULT eResult = AK_Success;
	eResult = m_SwitchEntries.Init( g_DefaultPoolId );
	if( eResult == AK_Success )
	{
		eResult = m_RTPCEntries.Init( g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_listRTPCSubscribers.Init( MIN_SIZE_ENTRIES, MAX_SIZE_ENTRIES, g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_listSwitchSubscribers.Init( MIN_SIZE_ENTRIES_LOW, MAX_SIZE_ENTRIES, g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_listRTPCSwitch.Init( 0, MAX_SIZE_ENTRIES, g_DefaultPoolId );
	}
	if( eResult == AK_Success )
	{
		eResult = m_mapRTPCLayers.Init( 0, MAX_SIZE_ENTRIES, g_DefaultPoolId );
	}

	//something went wrong, Terms the lists and return the error code
	if(eResult != AK_Success)
	{
		Term();
	}

	return eResult;
}

AKRESULT CAkRTPCMgr::Term()
{
	if ( m_listRTPCSubscribers.IsInitialized() )
	{
		for( AkListRTPCSubscribers::Iterator iter = m_listRTPCSubscribers.Begin(); iter != m_listRTPCSubscribers.End(); ++iter )
		{
			(*iter).Term();
		}
	}
	if ( m_listRTPCSwitch.IsInitialized() )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			(*iter).listRTPCSwitchSubscribers.Term();
			(*iter).ConversionTable.Unset();
		}
	}

	m_SwitchEntries.Term();
	m_RTPCEntries.Term();
	m_listRTPCSubscribers.Term();
	m_listSwitchSubscribers.Term();
	m_listRTPCSwitch.Term();
	m_mapRTPCLayers.Term();

	return AK_Success;
}

AKRESULT CAkRTPCMgr::AddSwitchRTPC(
		AkSwitchGroupID				in_switchGroup, 
		AkRtpcID					in_RTPC_ID,
		AkRTPCGraphPointInteger*	in_pArrayConversion,//NULL if none
		AkUInt32						in_ulConversionArraySize//0 if none
		)
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKRESULT eResult = AK_Success;

	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_switchGroup )
		{
			//Here replace its content
			//Unset before setting here
			//then return directly

			(*iter).RTPC_ID = in_RTPC_ID;
			(*iter).ConversionTable.Unset();
			return (*iter).ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, AkCurveScaling_None );
		}
	}

	//Was not found, then add it
	AkRTPCSwitchAssociation* p_switchLink = m_listRTPCSwitch.AddLast();
	if( p_switchLink )
	{
		p_switchLink->switchGroup = in_switchGroup;
		p_switchLink->RTPC_ID = in_RTPC_ID;
		eResult = p_switchLink->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, AkCurveScaling_None );
		if( eResult == AK_Success )
		{
			eResult = p_switchLink->listRTPCSwitchSubscribers.Init( 0, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
			if( eResult != AK_Success )
			{
				p_switchLink->ConversionTable.Unset();
				RemoveSwitchRTPC( in_switchGroup );
			}
		}
		else
		{
			RemoveSwitchRTPC( in_switchGroup );
		}
	}
	else
	{
		eResult = AK_Fail;
	}

	//Here, all switch that were on this switch must be redirected to RTPC subscription
	AkListSwitchSubscribers::IteratorEx iterEx = m_listSwitchSubscribers.BeginEx();
	while( iterEx != m_listSwitchSubscribers.End() )
    {
		if( (*iterEx).switchGroup == in_switchGroup )
		{
			CAkSwitchAware* pSwCntr = (*iterEx).pSwitch;
			iterEx = m_listSwitchSubscribers.Erase( iterEx );

			SubscribeSwitch( pSwCntr, in_switchGroup );
		}
		else
		{
			++iterEx;
		}
	}

	return eResult;
	
}

AKRESULT CAkRTPCMgr::RegisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID )
{
	return m_mapRTPCLayers.Insert( in_rtpcID, in_pLayer );
}

void CAkRTPCMgr::UnregisterLayer( CAkLayer* in_pLayer, AkRtpcID in_rtpcID )
{
	m_mapRTPCLayers.Remove( in_rtpcID, in_pLayer );
}

void CAkRTPCMgr::RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	for( AkListRTPCSwitch::IteratorEx iter = m_listRTPCSwitch.BeginEx(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_switchGroup )
		{
			(*iter).ConversionTable.Unset();

			//Shovel them in the normal path
			for( AkListRTPCSwitchSubscribers::Iterator iterSubs =  (*iter).listRTPCSwitchSubscribers.Begin(); iterSubs != (*iter).listRTPCSwitchSubscribers.End(); ++iterSubs )
			{
				SubscribeSwitch( *iterSubs, in_switchGroup, true );
			}

			(*iter).listRTPCSwitchSubscribers.Term();
			m_listRTPCSwitch.Erase( iter );
			return;
		}
	}
}

AKRESULT CAkRTPCMgr::SetRTPCInternal( AkRtpcID in_RTPCid, AkReal32 in_Value, CAkRegisteredObj * in_GameObj )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkRTPCKey l_key;
	l_key.m_pGameObj = in_GameObj;
	l_key.m_RTPC_id = in_RTPCid;

	AkReal32* pOldValue = m_RTPCEntries.Exists( l_key );
	//No update required
	if( pOldValue && *pOldValue == in_Value )
		return AK_Success;

	GameObjExceptArray l_ExceptArray;
	bool l_bCheckedExcept = false;

	for( AkListRTPCSubscribers::Iterator iter = m_listRTPCSubscribers.Begin(); iter != m_listRTPCSubscribers.End(); ++iter )
    {
		AkRTPCSubscription& rItem = *iter;

		// Does this subscription use the modified Game Parameter?
		bool bNeedsToBeProcessed = false;
		for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
		{
			if( iterCurves.pItem->RTPC_ID == in_RTPCid )
			{
				bNeedsToBeProcessed = true;
				break;
			}
		}

		if( bNeedsToBeProcessed )
		{
			if( (in_GameObj == rItem.TargetGameObject) 
				|| (in_GameObj == NULL) 
				|| (rItem.TargetGameObject == NULL)
				)
			{
				AKASSERT( rItem.pSubscriber );
				AkReal32 l_value = 0;

				if( rItem.eType == SubscriberType_CAkParameterNodeBase || rItem.eType == SubscriberType_CAkBus )
				{
					// Consider only global RTPCs for busses
					if ( in_GameObj != NULL && rItem.eType == SubscriberType_CAkBus )
						continue;

					CAkParameterNodeBase* pNode = reinterpret_cast<CAkParameterNodeBase*>(rItem.pSubscriber);
					if( pNode->IsPlaying() 
						|| ( 
							( ( rItem.ParamID >= RTPC_BypassFX0 )
								&& ( rItem.ParamID <= RTPC_BypassAllFX ) )
							&& rItem.eType == SubscriberType_CAkBus ) // FIXME
						)
					{
						// Evaluate the subscription's curves that are based on in_RTPCid, add their values
						for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
						{
							if( iterCurves.pItem->RTPC_ID == in_RTPCid )
							{
								l_value += iterCurves.pItem->ConversionTable.Convert( in_Value );
							}
						}

						GameObjExceptArray* pExceptArray = NULL;

						// Build exception array, but not for busses
						if( in_GameObj == NULL && rItem.eType != SubscriberType_CAkBus )
						{
							// Ensure to check for exceptions only once since it is a painful process.
							if( l_bCheckedExcept == false )
							{
								GetRTPCExceptions( in_RTPCid, l_ExceptArray );
								l_bCheckedExcept = true;
							}

							pExceptArray = &l_ExceptArray;
						}

						pNode->SetParamComplexFromRTPCManager(
							(void *) &rItem,
							rItem.ParamID,
							in_RTPCid,
							l_value,
							in_GameObj,
							pExceptArray
							);
					}
				}
				else if ( rItem.eType == SubscriberType_IAkRTPCSubscriber )
				{
					AkRTPCKey l_key;
					l_key.m_pGameObj = rItem.TargetGameObject;
					l_key.m_RTPC_id = in_RTPCid;

					// For each subscriber, the rule is:
					//   * If the game object is the one you registered for (either NULL or a specific object), then
					//     this is for you
					//   * If not, then if the game object is NULL (aka global RTPC), and *your* game object
					//     doesn't have a value for this RTPC, then this is for you too.
					if ( (in_GameObj == rItem.TargetGameObject) || ( in_GameObj == NULL && ! m_RTPCEntries.Exists( l_key ) ) )
					{
						// Evaluate the subscription's curves that are based on in_RTPCid, add their values
						// NOTE: Right now these objects support only one RTPC curve on each parameter. If they
						// ever support multiple curves, they will also probably support additive RTPC, in which
						// case this code will need to be modified to work in this new context.
						AKASSERT( rItem.Curves.Length() == 1 && (rItem.Curves.Begin()).pItem->RTPC_ID == in_RTPCid );
						l_value = (rItem.Curves.Begin()).pItem->ConversionTable.Convert( in_Value );

						// Notify the registered user of the change
						reinterpret_cast<IAkRTPCSubscriber*>( rItem.pSubscriber )->SetParam( static_cast<AkPluginParamID>(rItem.ParamID), &l_value, sizeof(l_value) );
					}
				}
				else if( rItem.eType == SubscriberType_PBI )
				{
					AkRTPCKey l_key;
					l_key.m_pGameObj = rItem.TargetGameObject;
					l_key.m_RTPC_id = in_RTPCid;

					// For each subscriber, the rule is:
					//   * If the game object is the one you registered for (either NULL or a specific object), then
					//     this is for you
					//   * If not, then if the game object is NULL (aka global RTPC), and *your* game object
					//     doesn't have a value for this RTPC, then this is for you too.
					if ( (in_GameObj == rItem.TargetGameObject) || ( in_GameObj == NULL && ! m_RTPCEntries.Exists( l_key ) ) )
					{
						for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
						{
							if( iterCurves.pItem->RTPC_ID == in_RTPCid )
							{
								l_value += iterCurves.pItem->ConversionTable.Convert( in_Value );
							}
						}

						// Notify the registered user of the change
						static_cast<CAkPBI*>( rItem.pSubscriber )->SetParam( static_cast<AkPluginParamID>(rItem.ParamID), &l_value, sizeof(l_value) );
					}
				}
				else
				{
					AKASSERT( rItem.eType == SubscriberType_CAkLayer );

					CAkLayer* pLayer = reinterpret_cast<CAkLayer*>(rItem.pSubscriber);
					if( pLayer->IsPlaying() )
					{
						// Evaluate the subscription's curves that are based on in_RTPCid, add their values
						for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
						{
							if( iterCurves.pItem->RTPC_ID == in_RTPCid )
							{
								l_value += iterCurves.pItem->ConversionTable.Convert( in_Value );
							}
						}

						if( in_GameObj == NULL )
						{
							// Ensure to check for exceptions only once since it is a painful process.
							if( l_bCheckedExcept == false )
							{
								GetRTPCExceptions( in_RTPCid, l_ExceptArray );
								l_bCheckedExcept = true;
							}
						}

						pLayer->SetParamComplexFromRTPCManager(
							(void *) &rItem,
							rItem.ParamID,
							in_RTPCid,
							l_value,
							in_GameObj,
							&l_ExceptArray
							);
					}
				}
			}
		}
	}

	if( in_GameObj != NULL )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			AkRTPCSwitchAssociation& rItem = *iter;

			if( rItem.RTPC_ID == in_RTPCid )
			{
				AkUInt32 l_newValue = rItem.ConversionTable.Convert( in_Value );

				//If there were no previous value or if the new value gives a different result, proceed.
				if( !pOldValue || rItem.ConversionTable.Convert( *pOldValue ) != l_newValue )
				{
					for( AkListRTPCSwitchSubscribers::Iterator iterSubs =  rItem.listRTPCSwitchSubscribers.Begin(); iterSubs != rItem.listRTPCSwitchSubscribers.End(); ++iterSubs )
					{
						(*iterSubs)->SetSwitch( (AkUInt32)l_newValue, in_GameObj );
					}
				}

				//TODO here; //+ Capture log RTPC value + RTPCID + SwitchSelected
				// There will be flooding problems here with this notifications, 
				// to be Discussed with PMs.
			}
			
		}
	}

	// Notify Layers
	for ( AkMultimapRTPCLayerRegistration::Iterator it = m_mapRTPCLayers.Begin(), itEnd = m_mapRTPCLayers.End();
		  it != itEnd;
		  ++it )
	{
		// Keys are sorted. If we're past in_RTPCid, we're done.
		if ( (*it).key > in_RTPCid )
			break;

		if ( (*it).key == in_RTPCid )
			(*it).item->OnRTPCChanged( in_GameObj, in_Value );
	}
		
	l_ExceptArray.Term();

	if( pOldValue )
	{
		*pOldValue = in_Value;
		return AK_Success;
	}
	else
	{
		AkReal32 * pValue = m_RTPCEntries.Set( l_key );
		if ( pValue )
		{
			*pValue = in_Value;
			return AK_Success;
		}
		else
		{
			return AK_Fail;
			}
		}
}

AKRESULT CAkRTPCMgr::SetSwitchInternal( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, CAkRegisteredObj *  in_GameObj )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkSwitchKey l_key;
	l_key.m_pGameObj = in_GameObj;
	l_key.m_SwitchGroup = in_SwitchGroup;
	MONITOR_SWITCHCHANGED( in_SwitchGroup, in_SwitchState, in_GameObj?in_GameObj->ID():AK_INVALID_GAME_OBJECT );

	AKRESULT eResult;
	AkSwitchStateID * pSwitchState = m_SwitchEntries.Set( l_key );
	if ( pSwitchState )
	{
		*pSwitchState = in_SwitchState;
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
	}

	for( AkListSwitchSubscribers::Iterator iter = m_listSwitchSubscribers.Begin(); iter != m_listSwitchSubscribers.End(); ++iter )
    {
		if( (*iter).switchGroup == in_SwitchGroup )
		{
			(*iter).pSwitch->SetSwitch( in_SwitchState, in_GameObj );
		}
	}

	return eResult;
}

void CAkRTPCMgr::GetRTPCExceptions( AkRtpcID in_RTPCid, GameObjExceptArray& io_ExceptArrayObj )
{
	AkAutoLock<CAkLock> RTPCLock( m_RTPCLock );

	for( AkMapRTPCEntries::Iterator iter = m_RTPCEntries.Begin(); iter != m_RTPCEntries.End(); ++iter )
	{
		if( (*iter).key.m_RTPC_id == in_RTPCid && (*iter).key.m_pGameObj != NULL )
		{
			io_ExceptArrayObj.AddLast( (*iter).key.m_pGameObj );
		}
	}
}

AkSwitchStateID CAkRTPCMgr::GetSwitch( AkSwitchGroupID in_SwitchGroup, CAkRegisteredObj *  in_GameObj /*= NULL*/ )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
	{
		if( (*iter).switchGroup == in_SwitchGroup )
		{
			AkRTPCKey l_key;
			l_key.m_pGameObj = in_GameObj;
			l_key.m_RTPC_id = (*iter).RTPC_ID;
			AkReal32* pValue = m_RTPCEntries.Exists( l_key );

			AkReal32 l_RTPCValue;
			if(pValue)
			{
				l_RTPCValue = *pValue;
			}
			else
			{
				l_RTPCValue = (*iter).ConversionTable.GetMidValue();
			}

			return static_cast<AkSwitchStateID>( (*iter).ConversionTable.Convert( l_RTPCValue ) );
		}
	}

	// Not found in RTPCSwitches, proceed normal way.

	AkSwitchKey l_key;
	l_key.m_pGameObj = in_GameObj;
	l_key.m_SwitchGroup = in_SwitchGroup;
	AkSwitchStateID* pSwitchState = m_SwitchEntries.Exists( l_key );

	AkSwitchStateID l_SwitchState = AK_DEFAULT_SWITCH_STATE;
	if(pSwitchState)
	{
		l_SwitchState = *pSwitchState;
	}
	else if( l_key.m_pGameObj != NULL )
	{
		l_key.m_pGameObj = NULL;
		pSwitchState = m_SwitchEntries.Exists( l_key );
		if(pSwitchState)
		{
			l_SwitchState = *pSwitchState;
		}
	}
	return l_SwitchState;
}

AkReal32 CAkRTPCMgr::GetRTPCConvertedValue(
	void *					in_pToken,
	CAkRegisteredObj *		in_GameObj,
	AkRtpcID				in_RTPCid
	)
{
	// NO LOCK as this is a method used in an outer lock. Necessary for reasonable performance in SetRTPCInternal.

	AkRTPCSubscription * pItem = (AkRTPCSubscription *) in_pToken;

	AkReal32 rtpcValue;
	bool bGameObjectSpecificValue;
	const bool bGotValue = ( in_RTPCid != AK_INVALID_RTPC_ID ) && GetRTPCValue( in_RTPCid, in_GameObj, rtpcValue, bGameObjectSpecificValue );

	AkReal32 fResult = 0;
	for( RTPCCurveArray::Iterator iterCurves = pItem->Curves.Begin(); iterCurves != pItem->Curves.End(); ++iterCurves )
	{
		if( in_RTPCid == AK_INVALID_RTPC_ID || iterCurves.pItem->RTPC_ID == in_RTPCid )
		{
			if ( ! bGotValue )
			{
				if ( in_RTPCid == AK_INVALID_RTPC_ID )
				{
					if ( ! GetRTPCValue( iterCurves.pItem->RTPC_ID, in_GameObj, rtpcValue, bGameObjectSpecificValue ) )
						rtpcValue = iterCurves.pItem->ConversionTable.GetMidValue();
				}
				else
				{
					rtpcValue = iterCurves.pItem->ConversionTable.GetMidValue();
				}
			}

			fResult += iterCurves.pItem->ConversionTable.Convert( rtpcValue );
		}
	}

	return fResult;
}

AkReal32 CAkRTPCMgr::GetRTPCConvertedValue(
		void*				in_pSubscriber,
		AkUInt32				in_ParamID,
		CAkRegisteredObj * 	in_GameObj
		)
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkReal32 l_ValueToReturn = DEFAULT_RTPC_VALUE;

	for( AkListRTPCSubscribers::Iterator iter = m_listRTPCSubscribers.Begin(); iter != m_listRTPCSubscribers.End(); ++iter )
    {
		AkRTPCSubscription& rItem = *iter;

		if( rItem.pSubscriber == in_pSubscriber )
		{
			if( rItem.ParamID == in_ParamID )
			{
				l_ValueToReturn = GetRTPCConvertedValue( (void*) &rItem, in_GameObj, AK_INVALID_RTPC_ID /* Process curves for all RTPC IDs */ );
				break;
			}
		}
	}

	return l_ValueToReturn;
}

bool CAkRTPCMgr::GetRTPCValue(
	AkRtpcID in_RTPC_ID,
	CAkRegisteredObj* in_GameObj,
	AkReal32& out_value,
	bool& out_bGameObjectSpecificValue
)
{
	AkRTPCKey key;
	key.m_pGameObj = in_GameObj;
	key.m_RTPC_id = in_RTPC_ID;

	AkReal32* pValue = m_RTPCEntries.Exists( key );

	if(pValue)
	{
		out_value = *pValue;
		out_bGameObjectSpecificValue = ( in_GameObj != NULL );
		return true;
	}
	else if( key.m_pGameObj != NULL ) // If object specific not found, at least get global one if available
	{
		out_bGameObjectSpecificValue = false;
		key.m_pGameObj = NULL;
		pValue = m_RTPCEntries.Exists( key );
		if(pValue)
		{
			out_value = *pValue;
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////
// Subscription Functions
///////////////////////////////////////////////////////////////////////////

AKRESULT CAkRTPCMgr::SubscribeRTPC(
		void*						in_pSubscriber,
		AkRtpcID					in_RTPC_ID,
		AkUInt32						in_ParamID,		//# of the param that must be notified on change
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32						in_ulConversionArraySize,
		CAkRegisteredObj * 			in_TargetGameObject,
		SubscriberType				in_eType
		)
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKASSERT(in_pSubscriber);

	//Suppose parameters are wrongs
	AKRESULT eResult = AK_InvalidParameter;

	if(in_pSubscriber)
	{
		AkRTPCSubscription* pSubscription = FindSubscription( in_pSubscriber, in_ParamID );
		if ( pSubscription )
		{
			// Remove existing entry for the specified curve
			for( RTPCCurveArray::Iterator iterCurves = pSubscription->Curves.Begin(); iterCurves != pSubscription->Curves.End(); ++iterCurves )
			{
				if( iterCurves.pItem->RTPCCurveID == in_RTPCCurveID )
				{
					iterCurves.pItem->ConversionTable.Unset();
					pSubscription->Curves.Erase( iterCurves );
					break;
				}
			}
		}
		else
		{
			// Create a new subscription
			pSubscription = m_listRTPCSubscribers.AddLast();
			if ( ! pSubscription )
			{
				eResult = AK_InsufficientMemory;
			}
			else
			{
				pSubscription->pSubscriber = in_pSubscriber;
				pSubscription->ParamID = in_ParamID;
				pSubscription->eType = in_eType;
				pSubscription->TargetGameObject = in_TargetGameObject;
			}
		}

		if ( pSubscription )
		{
			if( in_pArrayConversion && in_ulConversionArraySize )
			{
				RTPCCurve* pNewCurve = pSubscription->Curves.AddLast();
				if ( ! pNewCurve )
				{
					eResult = AK_InsufficientMemory;
				}
				else
				{
					pNewCurve->RTPC_ID = in_RTPC_ID;
					pNewCurve->RTPCCurveID = in_RTPCCurveID;
					eResult = pNewCurve->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );

					if ( eResult != AK_Success )
						pSubscription->Curves.RemoveLast();
				}
			}
		}

		if( eResult == AK_Success )
		{
			eResult = UpdateRTPCSubscriberInfo( in_pSubscriber );
		}
		else if ( pSubscription && pSubscription->Curves.IsEmpty() )
		{
			pSubscription->Term();
			m_listRTPCSubscribers.Remove( *pSubscription );
		}
	}

	return eResult;
}

AKRESULT CAkRTPCMgr::SubscribeSwitch(
		CAkSwitchAware*		in_pSwitch,
		AkSwitchGroupID		in_switchGroup,
		bool				in_bForceNoRTPC /*= false*/
		)
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKASSERT(in_pSwitch);

	if( !in_bForceNoRTPC )
	{
		for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
		{
			if( (*iter).switchGroup == in_switchGroup )
			{
				return (*iter).listRTPCSwitchSubscribers.AddLast( in_pSwitch ) ? AK_Success : AK_Fail;
			}
		}
	}

	//Suppose parameters are wrongs
	AKRESULT eResult = AK_InvalidParameter;

	if(in_pSwitch)
	{
		//First, remove the old version
		UnSubscribeSwitch( in_pSwitch );

		AkSwitchSubscription Item;
		Item.pSwitch = in_pSwitch;
		Item.switchGroup = in_switchGroup;

		eResult = m_listSwitchSubscribers.AddLast( Item ) ? AK_Success : AK_Fail;
	}

	return eResult;
}

AKRESULT CAkRTPCMgr::UnSubscribeRTPC( void* in_pSubscriber, AkUInt32 in_ParamID, AkUniqueID in_RTPCCurveID, bool* out_bMoreCurvesRemaining /* = NULL */ )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkListRTPCSubscribers::IteratorEx iter = m_listRTPCSubscribers.BeginEx();
	while( iter != m_listRTPCSubscribers.End() )
    {
		AkRTPCSubscription& rItem = *iter;
		if( rItem.pSubscriber == in_pSubscriber && rItem.ParamID == in_ParamID)
		{
			if ( out_bMoreCurvesRemaining )
				*out_bMoreCurvesRemaining = ! rItem.Curves.IsEmpty();

			for( RTPCCurveArray::Iterator iterCurves = rItem.Curves.Begin(); iterCurves != rItem.Curves.End(); ++iterCurves )
			{
				if( iterCurves.pItem->RTPCCurveID == in_RTPCCurveID )
				{
					iterCurves.pItem->ConversionTable.Unset();
					rItem.Curves.Erase( iterCurves );

					if ( rItem.Curves.IsEmpty() )
					{
						if ( out_bMoreCurvesRemaining )
							*out_bMoreCurvesRemaining = false;

						rItem.Term();
						m_listRTPCSubscribers.Erase( iter );
					}

					break;
				}
			}

			// Found the subscription but not the curve
			return AK_Success;
		}

		++iter;
	}

	if ( out_bMoreCurvesRemaining )
		*out_bMoreCurvesRemaining = false;

	return AK_Success;
}

AKRESULT CAkRTPCMgr::UnSubscribeRTPC( void* in_pSubscriber )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkListRTPCSubscribers::IteratorEx iter = m_listRTPCSubscribers.BeginEx();
	while( iter != m_listRTPCSubscribers.End() )
    {
		AkRTPCSubscription& rItem = *iter;;
		if( rItem.pSubscriber == in_pSubscriber )
		{
			rItem.Term();
			iter = m_listRTPCSubscribers.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return AK_Success;
}

void CAkRTPCMgr::UnSubscribeSwitch( CAkSwitchAware* in_pSwitch )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AkListSwitchSubscribers::IteratorEx iter = m_listSwitchSubscribers.BeginEx();
	while( iter != m_listSwitchSubscribers.End() )
    {
		AkSwitchSubscription& rItem = *iter;
		if( rItem.pSwitch == in_pSwitch )
		{
			iter = m_listSwitchSubscribers.Erase( iter );
			return;
		}
		else
		{
			++iter;
		}
	}

	for( AkListRTPCSwitch::Iterator iterRTPCSwitch = m_listRTPCSwitch.Begin(); iterRTPCSwitch != m_listRTPCSwitch.End(); ++iterRTPCSwitch )
    {
		AkRTPCSwitchAssociation& rItem = *iterRTPCSwitch;

		AkListRTPCSwitchSubscribers::IteratorEx iter2 = rItem.listRTPCSwitchSubscribers.BeginEx();
		while( iter2 != rItem.listRTPCSwitchSubscribers.End() )
		{
			if( (*iter2) == in_pSwitch )
			{
				iter2 = rItem.listRTPCSwitchSubscribers.Erase( iter2 );
				return;
			}
			else
			{
				++iter2;
			}
		}
	}
}

void CAkRTPCMgr::UnregisterGameObject(CAkRegisteredObj * in_GameObj)
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKASSERT( in_GameObj != NULL );
	if( in_GameObj == NULL )
	{
		return;
	}

	//Clear the subscriber list
	AkListRTPCSubscribers::IteratorEx iterRTPCSubs = m_listRTPCSubscribers.BeginEx();
	while( iterRTPCSubs != m_listRTPCSubscribers.End() )
    {
		AkRTPCSubscription& rItem = *iterRTPCSubs;

		if( rItem.TargetGameObject == in_GameObj )
		{
			rItem.Term();
			iterRTPCSubs = m_listRTPCSubscribers.Erase( iterRTPCSubs );
		}
		else
		{
			++iterRTPCSubs;
		}
	}

	//Clear its specific switches
	AkMapSwitchEntries::IteratorEx iterSwitch = m_SwitchEntries.BeginEx();
	while( iterSwitch != m_SwitchEntries.End() )
	{
		if( (*iterSwitch).key.m_pGameObj == in_GameObj )
		{
			iterSwitch = m_SwitchEntries.Erase( iterSwitch );
		}
		else
		{
			++iterSwitch;
		}
	}

	//Clear its specific RTPCs
	AkMapRTPCEntries::IteratorEx iterRTPC = m_RTPCEntries.BeginEx();
	while( iterRTPC != m_RTPCEntries.End() )
	{
		if( (*iterRTPC).key.m_pGameObj == in_GameObj )
		{
			iterRTPC = m_RTPCEntries.Erase( iterRTPC );
		}
		else
		{
			++iterRTPC;
		}
	}
}

AKRESULT CAkRTPCMgr::UpdateRTPCSubscriberInfo( void* in_pSubscriber )
{
//TODO Alessard Add an option to avoid calling UpdateRTPCSubscriberInfo too many times on load banks when miltiple RTPC are available
//The solution is probably to add two parameters to this function, including paramID and GameObject(and bool is necessary)
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	AKRESULT eResult = AK_Success;

	//get what he subscribed for

	for( AkListRTPCSubscribers::Iterator iter = m_listRTPCSubscribers.Begin(); iter != m_listRTPCSubscribers.End(); ++iter )
	{
		if( (*iter).pSubscriber == in_pSubscriber )
		{
			UpdateSubscription( *iter );
		}
	}

	return eResult;
}

void CAkRTPCMgr::UpdateSubscription( AkRTPCSubscription& in_rSubscription )
{
	if( in_rSubscription.eType == SubscriberType_CAkParameterNodeBase || in_rSubscription.eType == SubscriberType_CAkBus )
	{
		// Simply tell playing instances to recalc, way faster and way simplier than starting a serie of endless notifications
		reinterpret_cast<CAkParameterNodeBase*>(in_rSubscription.pSubscriber)->RecalcNotification();
	}
	else if ( in_rSubscription.eType == SubscriberType_IAkRTPCSubscriber )
	{
		AkReal32 l_RTPCValue = GetRTPCConvertedValue( &in_rSubscription, in_rSubscription.TargetGameObject, AK_INVALID_RTPC_ID );

		reinterpret_cast<IAkRTPCSubscriber*>( in_rSubscription.pSubscriber )->SetParam( 
				static_cast<AkPluginParamID>( in_rSubscription.ParamID ), 
				&l_RTPCValue,
				sizeof( l_RTPCValue )
				);
	}
	else if( in_rSubscription.eType == SubscriberType_PBI )
	{
		AkReal32 l_RTPCValue = GetRTPCConvertedValue( &in_rSubscription, in_rSubscription.TargetGameObject, AK_INVALID_RTPC_ID );

		static_cast<CAkPBI*>( in_rSubscription.pSubscriber )->SetParam( 
				static_cast<AkPluginParamID>( in_rSubscription.ParamID ), 
				&l_RTPCValue,
				sizeof( l_RTPCValue )
				);
	}
	else
	{
		AKASSERT( in_rSubscription.eType == SubscriberType_CAkLayer );

		// Simply tell playing instances to recalc, way faster and way simplier than starting a serie of endless notifications
		reinterpret_cast<CAkLayer*>(in_rSubscription.pSubscriber)->RecalcNotification();
	}
}

void CAkRTPCMgr::ResetSwitches( CAkRegisteredObj * in_GameObj )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	if( in_GameObj == NULL )
	{
		m_SwitchEntries.RemoveAll();
	}
	else
	{
		AkMapSwitchEntries::IteratorEx iterSwitch = m_SwitchEntries.BeginEx();
		while( iterSwitch != m_SwitchEntries.End() )
		{
			if( (*iterSwitch).key.m_pGameObj == in_GameObj )
			{
				iterSwitch = m_SwitchEntries.Erase( iterSwitch );	
			}
			else
			{
				++iterSwitch;
			}
		}
	}

	for( AkListSwitchSubscribers::Iterator iter = m_listSwitchSubscribers.Begin(); iter != m_listSwitchSubscribers.End(); ++iter )
    {
		AKASSERT( (*iter).pSwitch );
		(*iter).pSwitch->SetSwitch( DEFAULT_SWITCH_TYPE, in_GameObj );
	}

}

void CAkRTPCMgr::ResetRTPC( CAkRegisteredObj * in_GameObj )
{
	AkAutoLock<CAkLock> RTPCLock(m_RTPCLock);

	// Reset all values and then update everybody...
	if( in_GameObj == NULL )
	{
		m_RTPCEntries.RemoveAll();
	}
	else
	{
		AkMapRTPCEntries::IteratorEx iter = m_RTPCEntries.BeginEx();
		while( iter != m_RTPCEntries.End() )
		{
			if( (*iter).key.m_pGameObj == in_GameObj )
			{
				iter = m_RTPCEntries.Erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}

	for( AkListRTPCSubscribers::Iterator iter = m_listRTPCSubscribers.Begin(); iter != m_listRTPCSubscribers.End(); ++iter )
	{
		UpdateSubscription( *iter );
	}

	for( AkListRTPCSwitch::Iterator iter = m_listRTPCSwitch.Begin(); iter != m_listRTPCSwitch.End(); ++iter )
    {
		for( AkListRTPCSwitchSubscribers::Iterator iter2 = (*iter).listRTPCSwitchSubscribers.Begin(); iter2 != (*iter).listRTPCSwitchSubscribers.End(); ++iter2 )
		{
			(*iter2)->SetSwitch( DEFAULT_SWITCH_TYPE, in_GameObj );
		}
	}
}

CAkRTPCMgr::AkRTPCSubscription* CAkRTPCMgr::FindSubscription( void* in_pSubscriber, AkUInt32 in_ParamID )
{
	AkListRTPCSubscribers::IteratorEx iter = m_listRTPCSubscribers.BeginEx();
	while( iter != m_listRTPCSubscribers.End() )
    {
		if( (*iter).pSubscriber == in_pSubscriber && (*iter).ParamID == in_ParamID )
			return &(*iter);

		++iter;
	}

	return NULL;
}

