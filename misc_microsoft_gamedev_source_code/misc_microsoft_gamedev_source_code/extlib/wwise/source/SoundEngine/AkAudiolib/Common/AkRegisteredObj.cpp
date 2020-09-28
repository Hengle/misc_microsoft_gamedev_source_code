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
// AkRegisteredObj.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkRegisteredObj.h"
#include "AkDefault3DParams.h"
#include "AkAudioNode.h"
#include "AkPBI.h"
#include "AkAudiolibIndex.h"
#include "AkRTPCMgr.h"

#ifdef RVL_OS
#include "AkWiimoteMgr.h"
#endif

extern AkMemPoolId g_DefaultPoolId;

CAkRegisteredObj::CAkRegisteredObj( AkGameObjectID in_GameObjID )
	: m_GameObjID( in_GameObjID )
	, m_refCount( 1 )
#ifdef RVL_OS
	,m_PlayCount( 0 )
#endif
{
}

AKRESULT CAkRegisteredObj::Init()
{
	AKRESULT eResult = m_listModifiedNodes.Init( 0, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );

	m_PosEntry.pos = g_DefaultSoundPosition;
	m_PosEntry.uListenerIdx = AK_INVALID_LISTENER_INDEX;
	m_PosEntry.uListenerMask = AK_DEFAULT_LISTENER_MASK;
#ifdef RVL_OS
	m_PosEntry.uControllerActiveMask = AK_WII_MAIN_AUDIO_OUTPUT;
#endif
	for( int j = 0; j < AK_MAX_ENVIRONMENTS_PER_OBJ; ++j )
	{
		m_EnvironmentValues[j].EnvID = AK_INVALID_ENV_ID;
		m_EnvironmentValues[j].fControlValue = 0.0f;
#ifndef AK_OPTIMIZED
		m_EnvironmentValues[j].fUserData = 0.0f;
#endif
	}
	m_fDryLevelValue = 1.0f;

	memset( m_fObstructionValue, 0, sizeof(AkReal32) * AK_NUM_LISTENERS );
	memset( m_fOcclusionValue, 0, sizeof(AkReal32) * AK_NUM_LISTENERS );

	return eResult;
}

CAkRegisteredObj::~CAkRegisteredObj()
{
#ifdef RVL_OS
	AKASSERT( m_PlayCount == 0 );
#endif

	FreeModifiedNodes();

	m_listModifiedNodes.Term();
	m_SwitchHist.Term();
}

void CAkRegisteredObj::SetNodeAsModified(CAkAudioNode* in_pNode)
{
	AKASSERT(in_pNode);
	if( !m_listModifiedNodes.Exists( in_pNode->ID() ) )
	{
		m_listModifiedNodes.AddLast( in_pNode->ID() );
		// if the addlast fails, There will be inconsistent Resets since we cannot clean everything.
		// Everything will be ok starting on the moment the bank gets re-loaded.
	}
}

void CAkRegisteredObj::FillElementList( AkListNode& io_rlistNode )
{
	for( AkListNode::Iterator iter = m_listModifiedNodes.Begin(); iter != m_listModifiedNodes.End(); ++iter )
	{
		io_rlistNode.AddLast( *iter );
		//Too bad if it misses, but some node will not get the notification.
	}
}

void CAkRegisteredObj::FreeModifiedNodes()
{
	for( AkListNode::Iterator iter = m_listModifiedNodes.Begin(); iter != m_listModifiedNodes.End(); ++iter )
	{
		CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( *iter );
		if(pNode)
		{
			pNode->Unregister( this );
			pNode->Release();
		}
	}
	g_pRTPCMgr->UnregisterGameObject( this );
}

void CAkRegisteredObj::SetPosition( const AkSoundPosition & in_Position, AkUInt32 in_ulListenerIndex )
{
	if ( in_ulListenerIndex == AK_INVALID_LISTENER_INDEX )
		AKASSERT( !( ( in_Position.Orientation.X == 0.0 ) && ( in_Position.Orientation.Y == 0.0 ) && ( in_Position.Orientation.Z == 0.0 ) ) );

	m_PosEntry.pos = in_Position;
	m_PosEntry.uListenerIdx = in_ulListenerIndex;
}

void CAkRegisteredObj::SetActiveListeners( AkUInt32 in_uListenerMask )
{
	m_PosEntry.uListenerMask = in_uListenerMask;
}

#ifdef RVL_OS
void CAkRegisteredObj::SetActiveControllers( AkUInt32 in_uActiveControllerMask )
{
	if( m_PlayCount != 0 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001
		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & in_uActiveControllerMask) && !(Remotemask & m_PosEntry.uControllerActiveMask) )
			{
				//Activating
				CAkWiimoteMgr::IncrementSpeakerActivityCount( uController );
			}
			else if( !(Remotemask & in_uActiveControllerMask) && (Remotemask & m_PosEntry.uControllerActiveMask) )
			{
				//Deactivating
				CAkWiimoteMgr::DecrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
	m_PosEntry.uControllerActiveMask = in_uActiveControllerMask;
}
#endif

AKRESULT CAkRegisteredObj::SetGameObjectEnvironmentsValues( 
		AkEnvironmentValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		)
{
	if( in_uNumEnvValues > AK_MAX_ENVIRONMENTS_PER_OBJ )
		return AK_Fail;

	if( in_aEnvironmentValues && in_uNumEnvValues )
	{
		// Copying the Valid Environments (max num env : AK_MAX_ENVIRONMENTS_PER_OBJ)
		AKPLATFORM::AkMemCpy( 
			m_EnvironmentValues,
			in_aEnvironmentValues, 
			in_uNumEnvValues*sizeof( AkEnvironmentValue ) 
			);
	}

	for( int i = in_uNumEnvValues; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
	{
		m_EnvironmentValues[i].EnvID = AK_INVALID_ENV_ID;
		m_EnvironmentValues[i].fControlValue = 0.0f;
#ifndef AK_OPTIMIZED
		m_EnvironmentValues[i].fUserData = 0.0f;
#endif
	}

	return AK_Success;
}

AKRESULT CAkRegisteredObj::SetGameObjectDryLevelValue(
		AkReal32			in_fControlValue
		)
{
	m_fDryLevelValue = in_fControlValue;

	return AK_Success;
}

AKRESULT CAkRegisteredObj::SetObjectObstructionAndOcclusion(
		AkUInt32			in_uListener,
		AkReal32			in_fObstructionValue,
		AkReal32			in_fOcclusionValue
		)
{
	if(in_uListener >= AK_NUM_LISTENERS)
		return AK_Fail;

	m_fObstructionValue[in_uListener] = in_fObstructionValue;
	m_fOcclusionValue[in_uListener] = in_fOcclusionValue;

	return AK_Success;
}

#ifdef RVL_OS
void CAkRegisteredObj::IncrementGameObjectPlayCount()
{
	++m_PlayCount;
	AKASSERT( m_PlayCount ); // Should never be zero here.
	if( m_PlayCount == 1 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001

		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & m_PosEntry.uControllerActiveMask) )
			{
				//Activating
				CAkWiimoteMgr::IncrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
}

void CAkRegisteredObj::DecrementGameObjectPlayCount()
{
	--m_PlayCount;
	if( m_PlayCount == 0 )
	{
		AKASSERT( AK_WII_REMOTE_3 == 0x0008 );// to make sure nobody modified this define
		AkUInt32 Remotemask = AK_WII_REMOTE_0; //0x0001

		for( AkUInt32 uController = 0; uController < WPAD_MAX_CONTROLLERS; ++uController )
		{
			if( (Remotemask & m_PosEntry.uControllerActiveMask) )
			{
				//Deactivating
				CAkWiimoteMgr::DecrementSpeakerActivityCount( uController );
			}
			Remotemask = Remotemask << 1;
		}
	}
}
#endif

