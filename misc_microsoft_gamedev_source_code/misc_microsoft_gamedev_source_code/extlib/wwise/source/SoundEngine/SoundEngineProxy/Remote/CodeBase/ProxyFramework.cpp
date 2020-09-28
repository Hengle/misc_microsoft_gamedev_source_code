/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/


#include "stdafx.h"

#include "ProxyFramework.h"

#include "EventProxyConnected.h"
#include "ActionProxyConnected.h"
#include "StateProxyConnected.h"
#include "AttenuationProxyConnected.h"
#include "SoundProxyConnected.h"
#include "RanSeqContainerProxyConnected.h"
#include "SwitchContainerProxyConnected.h"
#include "LayerContainerProxyConnected.h"
#include "ActorMixerProxyConnected.h"
#include "BusProxyConnected.h"
#include "LayerProxyConnected.h"
#include "FeedbackBusProxyConnected.h"	
#include "FeedbackNodeProxyConnected.h"
#include "AkCritical.h"

#include "CommandData.h"
#include "CommandDataSerializer.h"

#include <new>
#undef new

AkExternalProxyHandlerCallback g_pExternalProxyHandler = NULL;

ProxyFramework::ProxyFramework( AkMemPoolId in_pool )
	: m_pool( in_pool )
{
	ProxyCommandData::CommandData::s_poolID = m_pool;
}

ProxyFramework::~ProxyFramework()
{
}

void ProxyFramework::Destroy()
{
	AkDelete( m_pool, this );
}

void ProxyFramework::Init()
{
	m_id2ProxyConnected.Init( m_pool );
}

void ProxyFramework::Term()
{
	m_id2ProxyConnected.Term();
}

void ProxyFramework::SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel )
{
	m_monitorProxy.SetNotificationChannel( in_pNotificationChannel );
}

AK::Comm::IRendererProxy* ProxyFramework::GetRendererProxy()
{
	return &m_rendererProxy.GetLocalProxy();
}

void ProxyFramework::HandleExecute( const AkUInt8* in_pData, AK::IWriteBytes* in_pReturnData )
{
	ProxyCommandData::CommandData proxyCmdData;

	CommandDataSerializer serializer;
	serializer.Deserializing( in_pData );

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( serializer );
		serializer.Get( proxyCmdData );
	}

	if( proxyCmdData.m_commandType == ProxyCommandData::TypeObjectProxyStore )
	{
		ProcessProxyStoreCommand( serializer );
	}
	else if( proxyCmdData.m_commandType == ProxyCommandData::TypeObjectProxy )
	{
		ObjectProxyCommandData::CommandData objectData;
		
		{
			CommandDataSerializer::AutoSetDataPeeking peekGate( serializer );
			serializer.Get( objectData );
		}

		ObjectProxyConnectedWrapper* pProxyWrapper = m_id2ProxyConnected.Exists( objectData.m_proxyInstancePtr );
		//WG-5492 Removed this assert, it occurs when connecting on a non-interactive music system with Wwise and then trying to add some illegal content.
		//AKASSERT( pProxyWrapper );


		CommandDataSerializer returnSerializer;
		if( pProxyWrapper )
		{
			pProxyWrapper->GetObjectProxyConnected()->HandleExecute( serializer, returnSerializer );
		}

		long lDummy = 0;
		in_pReturnData->WriteBytes( returnSerializer.GetWrittenBytes(), returnSerializer.GetWrittenSize(), lDummy );
	}
	else if( proxyCmdData.m_commandType == ProxyCommandData::TypeRendererProxy )
	{
		CommandDataSerializer returnSerializer;

		m_rendererProxy.HandleExecute( serializer, returnSerializer );

		long lDummy = 0;
		in_pReturnData->WriteBytes( returnSerializer.GetWrittenBytes(), returnSerializer.GetWrittenSize(), lDummy );
	}
	else if( proxyCmdData.m_commandType == ProxyCommandData::TypeStateMgrProxy )
	{
		CommandDataSerializer returnSerializer;

		m_stateProxy.HandleExecute( serializer, returnSerializer );

		long lDummy = 0;
		in_pReturnData->WriteBytes( returnSerializer.GetWrittenBytes(), returnSerializer.GetWrittenSize(), lDummy );
	}
	else if( proxyCmdData.m_commandType == ProxyCommandData::TypeALMonitorProxy )
	{
		CommandDataSerializer returnSerializer;

		m_monitorProxy.HandleExecute( serializer, returnSerializer );

		long lDummy = 0;
		in_pReturnData->WriteBytes( returnSerializer.GetWrittenBytes(), returnSerializer.GetWrittenSize(), lDummy );
	}
	else
	{
		AKASSERT( !"Invalid proxy command." );
	}
}

#define PROXY_CAST_ALLOCATION_CONSTRUCTOR( _Creation_Type_, _What_ ) \
	pProxyItem = (ID2ProxyConnected::Item *)  AkAlloc( m_pool, lProxyItemOffset + sizeof( _Creation_Type_ ) ); \
	if( pProxyItem )\
	{\
		::new( &(pProxyItem->Assoc.item) ) _What_;\
	}

#define PROXY_CAST_ALLOCATION( _Creation_Type_ )		PROXY_CAST_ALLOCATION_CONSTRUCTOR( _Creation_Type_, _Creation_Type_( create.m_objectID ) )

#define PROXY_CAST_ALLOCATION_ACTION( _Action_Type_ )	PROXY_CAST_ALLOCATION_CONSTRUCTOR( _Action_Type_, _Action_Type_( in_eActionType, in_actionID ) )


void ProxyFramework::ProcessProxyStoreCommand( CommandDataSerializer& io_rSerializer )
{
	ObjectProxyStoreCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( io_rSerializer );
		io_rSerializer.Get( cmdData );
	}

	if( cmdData.m_methodID == ObjectProxyStoreCommandData::MethodCreate )
	{
		ObjectProxyStoreCommandData::Create create;
		io_rSerializer.Get( create );
		
		ID2ProxyConnected::Item * pProxyItem = NULL;
		const long lProxyItemOffset = offsetof( ID2ProxyConnected::Item, Assoc.item );

		switch( create.m_eObjectType )
		{
		case ObjectProxyStoreCommandData::TypeSound:
			PROXY_CAST_ALLOCATION( SoundProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeEvent:
			PROXY_CAST_ALLOCATION( EventProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeAction:
			pProxyItem = CreateAction( create.m_proxyInstancePtr, create.m_objectID, create.m_actionType );
			break;

		case ObjectProxyStoreCommandData::TypeState:
			PROXY_CAST_ALLOCATION_CONSTRUCTOR( StateProxyConnected, StateProxyConnected( create.m_objectID, false, create.m_StateGroupID ) );
			break;

		case ObjectProxyStoreCommandData::TypeCustomState:
			PROXY_CAST_ALLOCATION_CONSTRUCTOR( StateProxyConnected, StateProxyConnected( create.m_objectID, true, create.m_StateGroupID ) );
			break;

		case ObjectProxyStoreCommandData::TypeRanSeqContainer:
			PROXY_CAST_ALLOCATION( RanSeqContainerProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeSwitchContainer:
			PROXY_CAST_ALLOCATION( SwitchContainerProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeActorMixer:
			PROXY_CAST_ALLOCATION( ActorMixerProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeBus:
			PROXY_CAST_ALLOCATION( BusProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeLayerContainer:
			PROXY_CAST_ALLOCATION( LayerContainerProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeLayer:
			PROXY_CAST_ALLOCATION( LayerProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeAttenuation:
			PROXY_CAST_ALLOCATION( AttenuationProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeFeedbackBus:
			PROXY_CAST_ALLOCATION( FeedbackBusProxyConnected );
			break;
		case ObjectProxyStoreCommandData::TypeFeedbackNode:
			PROXY_CAST_ALLOCATION( FeedbackNodeProxyConnected );
			break;

		default:
			if( g_pExternalProxyHandler != NULL )
			{
				g_pExternalProxyHandler( create, pProxyItem, lProxyItemOffset, m_pool );
			}
			break;
		}

		if ( pProxyItem )
		{
			pProxyItem->Assoc.key = create.m_proxyInstancePtr;
			*m_id2ProxyConnected.Set( pProxyItem );
		}
	}
	else if( cmdData.m_methodID == ObjectProxyStoreCommandData::MethodRelease )
	{
		ObjectProxyStoreCommandData::Release releaseData;
		io_rSerializer.Get( releaseData );

		// It is possible to not fins them as it can have failed on creation, either by 
		// lack of memory or by trying to construct items that the SE is nto aware of ( AKA:IM )
		//AKASSERT( m_id2ProxyConnected.Exists( releaseData.m_proxyInstancePtr ) );

		m_id2ProxyConnected.Unset( releaseData.m_proxyInstancePtr );
	}
}

ProxyFramework::ID2ProxyConnected::Item* ProxyFramework::CreateAction( AkUInt32 in_proxyInstancePtr, AkUniqueID in_actionID, AkActionType in_eActionType )
{
	ID2ProxyConnected::Item* pProxyItem = NULL;
	const long lProxyItemOffset = offsetof( ID2ProxyConnected::Item, Assoc.item );

	const AkUInt32 pureActionType = in_eActionType & ACTION_TYPE_ACTION;

	switch( pureActionType )
	{
	case ACTION_TYPE_PLAY:
		PROXY_CAST_ALLOCATION_ACTION( ActionPlayProxyConnected );
		break;
	case ACTION_TYPE_STOP:
		PROXY_CAST_ALLOCATION_ACTION( ActionStopProxyConnected );
		break;
	case ACTION_TYPE_PAUSE:
		PROXY_CAST_ALLOCATION_ACTION( ActionPauseProxyConnected );
		break;
	case ACTION_TYPE_RESUME:
		PROXY_CAST_ALLOCATION_ACTION( ActionResumeProxyConnected );
		break;
	case ACTION_TYPE_BREAK:
		PROXY_CAST_ALLOCATION_ACTION( ActionBreakProxyConnected );
		break;
	case ACTION_TYPE_MUTE:
	case ACTION_TYPE_UNMUTE:
		PROXY_CAST_ALLOCATION_ACTION( ActionMuteProxyConnected );
		break;
	case ACTION_TYPE_SETPITCH:
	case ACTION_TYPE_RESETPITCH:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetPitchProxyConnected );
		break;
	case ACTION_TYPE_SETVOLUME:
	case ACTION_TYPE_RESETVOLUME:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetVolumeProxyConnected );
		break;
	case ACTION_TYPE_SETLFE:
	case ACTION_TYPE_RESETLFE:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetLFEProxyConnected );
		break;
	case ACTION_TYPE_SETLPF:
	case ACTION_TYPE_RESETLPF:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetLPFProxyConnected );
		break;
	case ACTION_TYPE_USESTATE:
	case ACTION_TYPE_UNUSESTATE:
		PROXY_CAST_ALLOCATION_ACTION( ActionUseStateProxyConnected );
		break;
	case ACTION_TYPE_BYPASSFX:
	case ACTION_TYPE_RESETBYPASSFX:
		PROXY_CAST_ALLOCATION_ACTION( ActionBypassFXProxyConnected );
		break;
	case ACTION_TYPE_SETSTATE:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetStateProxyConnected );
		break;
	case ACTION_TYPE_SETSWITCH:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetSwitchProxyConnected );
		break;
	case ACTION_TYPE_TRIGGER:
		PROXY_CAST_ALLOCATION_ACTION( ActionTriggerProxyConnected );
		break;
	case ACTION_TYPE_SETRTPC:
		PROXY_CAST_ALLOCATION_ACTION( ActionSetRTPCProxyConnected );
		break;
	case ACTION_TYPE_STOPEVENT:
	case ACTION_TYPE_PAUSEEVENT:
	case ACTION_TYPE_RESUMEEVENT:
		PROXY_CAST_ALLOCATION_ACTION( ActionEventProxyConnected );
		break;

	default:
		AKASSERT( !"Action type not supported yet." );
	}

	return pProxyItem;
}

void ProxyFramework::ControllerDisconnected()
{
	// Need tp release all the proxies.
	CAkFunctionCritical SpaceSetAsCritical; // object destruction needs to be protected

	m_id2ProxyConnected.RemoveAll();
}
