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

#include <AK/Comm/IProxyFrameworkConnected.h>
#include <AK/Comm/ICommandChannelHandler.h>
#include <AK/Comm/ICommunicationCentralNotifyHandler.h>
#include "RendererProxyConnected.h"
#include "StateMgrProxyConnected.h"
#include "ALMonitorProxyConnected.h"
#include "ObjectProxyConnected.h"

#include "AkHashList.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkPrivateTypes.h"
#include "AkAction.h"

class CommandDataSerializer;

class ProxyFramework 
	: public CAkObject
	, public AK::Comm::IProxyFrameworkConnected
{
public:
	ProxyFramework( AkMemPoolId in_pool );
	virtual ~ProxyFramework();

	// IProxyFramework members
	virtual void Destroy();

	virtual void Init();
	virtual void Term();

	virtual void SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel );
	
	virtual AK::Comm::IRendererProxy* GetRendererProxy();

	// ICommandChannelHandler members
	virtual void HandleExecute( const AkUInt8* in_pData, AK::IWriteBytes* in_pReturnData );

	// ICommunicationCentralNotifyHandler members
	virtual void ControllerDisconnected();


	typedef AkHashList< AkUInt32, ObjectProxyConnectedWrapper, 31 > ID2ProxyConnected;
private:
	void ProcessProxyStoreCommand( CommandDataSerializer& io_rSerializer );
	ID2ProxyConnected::Item * CreateAction( AkUInt32 in_proxyInstancePtr, AkUniqueID in_actionID, AkActionType in_eActionType );

	ID2ProxyConnected m_id2ProxyConnected;

	RendererProxyConnected m_rendererProxy;
	StateMgrProxyConnected m_stateProxy;
	ALMonitorProxyConnected m_monitorProxy;

	AkMemPoolId m_pool;
};

namespace ObjectProxyStoreCommandData
{
	struct Create;
}

typedef void (*AkExternalProxyHandlerCallback)( ObjectProxyStoreCommandData::Create& in_Create, ProxyFramework::ID2ProxyConnected::Item *& out_pProxyItem, const long in_lProxyItemOffset, AkMemPoolId in_PoolID );
