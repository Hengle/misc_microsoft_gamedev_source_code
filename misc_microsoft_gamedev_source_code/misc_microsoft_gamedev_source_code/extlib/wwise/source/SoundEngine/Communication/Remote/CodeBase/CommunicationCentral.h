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

#include <AK/Comm/ICommunicationCentral.h>
#include "DiscoveryChannel.h"
#include "ControllerChannel.h"
#include "CommandChannel.h"
#include "NotificationChannel.h"
#include "IChannelsHolder.h"

#include <AK/Tools/Common/AkObject.h>

class CommunicationCentral 
	: public CAkObject
	, public AK::Comm::ICommunicationCentral
	, public IChannelsHolder
{
public:
	CommunicationCentral( AkMemPoolId in_pool );
	~CommunicationCentral();

	// ICommunicationCentral members
	virtual void Destroy();

	virtual bool Init( AK::Comm::ICommunicationCentralNotifyHandler* in_pNotifyHandler, AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler );
	virtual void Term();

	virtual void Process();

	virtual AK::Comm::INotificationChannel* GetNotificationChannel();

	// IChannelsHolder members
	virtual void ControllerConnected();
	virtual void ControllerDisconnected();

	virtual bool PrepareChannels( const char* in_pszControllerName );
	virtual bool ResetChannels();
	virtual void WaitForChannelsReady();

private:
	DiscoveryChannel m_discoveryChannel;
	ControllerChannel m_controllerChannel;
	CommandChannel m_commandChannel;
	NotificationChannel m_notifChannel;

	bool m_bWaitingForChannelsReady;

	AK::Comm::ICommunicationCentralNotifyHandler* m_pNotifyHandler;

	AkMemPoolId m_pool;

	static AkUInt32 s_uiSupportedProtocolVersion;
};
