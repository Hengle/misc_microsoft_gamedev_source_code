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

// AkQueryParameters.cpp : Get some parameters values from the sound engine
//
#include "stdafx.h"

#include "Ak3DListener.h"
#include "AkAudioLib.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkAudioMgr.h"
#include "AkCritical.h"
#include "AkEnvironmentsMgr.h"
#include "AkEvent.h"
#include "AkStateMgr.h"
#include "AkRegistryMgr.h"
#include "AkRegisteredObj.h"
#include "AkParameterNode.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>


namespace AK
{
	namespace SoundEngine
	{
		namespace Query
		{
			AKRESULT GetPosition( 
				AkGameObjectID in_GameObj, 
				AkSoundPosition& out_rPosition
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				AkSoundPositionEntry soundPos = pObj->GetPosition();
				pObj->Release();

				out_rPosition = soundPos.pos;
				return AK_Success;
			}

			AKRESULT GetActiveListeners(
				AkGameObjectID in_GameObj,			///< Game object.
				AkUInt32& out_ruListenerMask		///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				AkSoundPositionEntry soundPos = pObj->GetPosition();
				pObj->Release();

				out_ruListenerMask = soundPos.uListenerMask;
				return AK_Success;
			}

			AKRESULT GetListenerPosition( 
				AkUInt32 in_ulIndex,
				AkListenerPosition& out_rPosition
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return CAkListener::GetListenerPosition( in_ulIndex, out_rPosition );
			}

			AKRESULT GetListenerSpatialization(
				AkUInt32 in_ulIndex,						///< Listener index. 
				bool& out_rbSpatialized,
				AkSpeakerVolumes& out_rVolumeOffsets
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				return CAkListener::GetListenerSpatialization( in_ulIndex, out_rbSpatialized, out_rVolumeOffsets );
			}

			AKRESULT GetRTPCValue( 
				AkRtpcID in_RTPCid, 
				AkGameObjectID in_GameObj,
				AkReal32& out_rfValue, 
				bool& out_rbGlobal
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				bool bObjectSpecific;
				bool bResult = g_pRTPCMgr->GetRTPCValue( in_RTPCid, pObj, out_rfValue, bObjectSpecific );
				out_rbGlobal = !bObjectSpecific;
				pObj->Release();
				return (bResult == true ? AK_Success : AK_Fail);
			}

			AKRESULT GetRTPCValue( 
				AkLpCtstr in_pszRTPCName, 
				AkGameObjectID in_GameObj,
				AkReal32& out_rfValue, 
				bool&	out_rbGlobal
				)
			{
				AkRtpcID id = AK::SoundEngine::GetIDFromString( in_pszRTPCName );
				if ( id == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetRTPCValue( id, in_GameObj, out_rfValue, out_rbGlobal );
			}

			AKRESULT GetSwitch( 
				AkSwitchGroupID in_SwitchGroup, 
				AkGameObjectID in_GameObj,
				AkSwitchStateID& out_rSwitchState
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				out_rSwitchState = g_pRTPCMgr->GetSwitch( in_SwitchGroup, pObj );
				pObj->Release();
				return AK_Success;
			}

			AKRESULT GetSwitch( 
				AkLpCtstr in_pstrSwitchGroupName, 
				AkGameObjectID in_GameObj,
				AkSwitchStateID& out_rSwitchState
				)
			{
				AkSwitchGroupID switchGroup = AK::SoundEngine::GetIDFromString( in_pstrSwitchGroupName );
				if ( switchGroup == AK_INVALID_RTPC_ID )
					return AK_IDNotFound;

				return GetSwitch( switchGroup, in_GameObj, out_rSwitchState );
			}

			AKRESULT GetState( 
				AkStateGroupID in_StateGroup, 
				AkStateID& out_rState )
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				out_rState = g_pStateMgr->GetState( in_StateGroup );
				return AK_Success;
			}

			AKRESULT GetState( 
				AkLpCtstr in_pstrStateGroupName, 
				AkStateID& out_rState )
			{
				AkStateGroupID stateGroup = AK::SoundEngine::GetIDFromString( in_pstrStateGroupName );
				return GetState( stateGroup, out_rState );
			}

			AKRESULT GetGameObjectEnvironmentsValues( 
				AkGameObjectID		in_GameObj,					///< the unique object ID
				AkEnvironmentValue*	out_paEnvironmentValues,	///< variable-size array of AkEnvironmentValue(s)
				AkUInt32&			io_ruNumEnvValues			///< number of elements in struct
				)
			{
				AKRESULT eReturn = AK_Success;

				if( io_ruNumEnvValues == 0 || !out_paEnvironmentValues )
					return AK_InvalidParameter;

				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				const AkEnvironmentValue* pEnvValues = pObj->GetEnvironmentValues();
				pObj->Release();

				AkUInt32 uNumEnvRet = 0;
				for(int iEnv=0; iEnv<AK_MAX_ENVIRONMENTS_PER_OBJ; ++iEnv)
				{
					if( pEnvValues[iEnv].EnvID == AK_INVALID_ENV_ID )
						break;

					uNumEnvRet++;
				}

				if( io_ruNumEnvValues < uNumEnvRet )
					eReturn = AK_PartialSuccess;

				io_ruNumEnvValues = AkMin( io_ruNumEnvValues, uNumEnvRet );
				AkMemCpy( out_paEnvironmentValues, (void*)pEnvValues, io_ruNumEnvValues * sizeof(AkEnvironmentValue) );
				return eReturn;
			}

			AKRESULT GetGameObjectDryLevelValue( 
				AkGameObjectID		in_GameObj,			///< the unique object ID
				AkReal32&			out_rfControlValue	///< control value for dry level
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_GameObj );
				if( !pObj )
					return AK_IDNotFound;

				out_rfControlValue = pObj->GetDryLevelValue();
				pObj->Release();

				return AK_Success;
			}

			AKRESULT GetEnvironmentVolume( 
				AkEnvID				in_FXParameterSetID,	///< Environment ID.
				AkReal32&			out_rfVolume			///< Volume control value.
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				out_rfVolume = g_pEnvironmentMgr->GetEnvironmentVolume( in_FXParameterSetID );
				return AK_Success;
			}

			AKRESULT GetEnvironmentBypass(
				AkEnvID	in_FXParameterSetID, 
				bool&	out_rbIsBypassed
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				out_rbIsBypassed = g_pEnvironmentMgr->IsBypassed( in_FXParameterSetID );
				return AK_Success;
			}

			AKRESULT GetObjectObstructionAndOcclusion(  
				AkGameObjectID in_ObjectID,				///< Game object ID.
				AkUInt32 in_uListener,					///< Listener ID.
				AkReal32& out_rfObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
				AkReal32& out_rfOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkRegisteredObj * pObj = g_pRegistryMgr->GetObjAndAddref( in_ObjectID );
				if( !pObj )
					return AK_IDNotFound;

				out_rfObstructionLevel = pObj->GetObjectObstructionValue( in_uListener );
				out_rfOcclusionLevel = pObj->GetObjectOcclusionValue( in_uListener );
				pObj->Release();

				return AK_Success;
			}

			// Advanced functions

			AKRESULT QueryAudioObjectIDs(
				AkUniqueID in_eventID,
				AkUInt32& io_ruNumItems,
				AkObjectInfo* out_aObjectInfos 
				)
			{
				if( io_ruNumItems && !out_aObjectInfos )
					return AK_InvalidParameter;

				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkEvent* pEvent = g_pIndex->m_idxEvents.GetPtrAndAddRef( in_eventID );
				if( !pEvent )
					return AK_IDNotFound;

				AKRESULT eResult = pEvent->QueryAudioObjectIDs( io_ruNumItems, out_aObjectInfos );
				pEvent->Release();
				return eResult;
			}

			AKRESULT QueryAudioObjectIDs(
				AkLpCtstr in_pszEventName,
				AkUInt32& io_ruNumItems,
				AkObjectInfo* out_aObjectInfos 
				)
			{
				AkUniqueID eventID = AK::SoundEngine::GetIDFromString( in_pszEventName );
				if ( eventID == AK_INVALID_UNIQUE_ID )
					return AK_IDNotFound;

				return QueryAudioObjectIDs( eventID, io_ruNumItems, out_aObjectInfos );
			}

			AKRESULT GetPositioningInfo( 
				AkUniqueID in_ObjectID,
				AkPositioningInfo& out_rPositioningInfo 
				)
			{
				CAkFunctionCritical SetSpaceAsCritical; //use global lock
				CAkAudioNode * pObj = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_ObjectID );
				if( !pObj )
					return AK_IDNotFound;

				if( pObj->NodeCategory() == AkNodeCategory_Bus )
				{
					pObj->Release();
					return AK_NotCompatible;
				}

				CAkParameterNode* pParam = (CAkParameterNode *)( pObj );
				AKRESULT eResult = pParam->GetStatic3DParams( out_rPositioningInfo );
				pObj->Release();

				return eResult;
			}


		} // namespace Query
	} // namespace SoundEngine
} // namespace AK
