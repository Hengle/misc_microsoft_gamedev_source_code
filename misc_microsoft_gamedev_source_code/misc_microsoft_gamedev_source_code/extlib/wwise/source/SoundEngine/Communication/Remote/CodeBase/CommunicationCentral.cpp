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


#include "stdafx.h"

#include "CommunicationCentral.h"
#include "Network.h"
#include "GameSocket.h"
#include <AK/Comm/ICommunicationCentralNotifyHandler.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////
AkUInt32 CommunicationCentral::s_uiSupportedProtocolVersion = 2;//
//////////////////////////////////////////////////////////////////


CommunicationCentral::CommunicationCentral( AkMemPoolId in_pool )
	: m_controllerChannel( this, in_pool )
	, m_commandChannel( in_pool )
	, m_bWaitingForChannelsReady( false )
	, m_pNotifyHandler( NULL )
	, m_pool( in_pool )
{
}

CommunicationCentral::~CommunicationCentral()
{
}

void CommunicationCentral::Destroy()
{
	AkDelete( m_pool, this );
}

bool CommunicationCentral::Init( AK::Comm::ICommunicationCentralNotifyHandler* in_pNotifyHandler, AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler )
{
	Network::Init( m_pool );

	m_pNotifyHandler = in_pNotifyHandler;
	m_commandChannel.SetCommandChannelHandler( in_pCmdChannelHandler );
	DiscoveryChannel::SetProtocolVersion( s_uiSupportedProtocolVersion );
	
	return m_commandChannel.Init()
			&& m_notifChannel.Init()
			&& m_controllerChannel.Init()
			&& m_discoveryChannel.Init();
}

void CommunicationCentral::Term()
{
	m_discoveryChannel.Term();
	m_controllerChannel.Term();
	m_notifChannel.Term();
	m_commandChannel.Term();

	Network::Term();
}

void CommunicationCentral::Process()
{
	// Go through the channels and call process
	m_discoveryChannel.Process();
	m_controllerChannel.Process();
	m_notifChannel.Process();
	m_commandChannel.Process();

	if( m_bWaitingForChannelsReady )
	{
		if( m_notifChannel.IsReady()
			&& m_commandChannel.IsReady() )
		{
			m_controllerChannel.SendChannelsReady();

			m_bWaitingForChannelsReady = false;
		}
	}
}

AK::Comm::INotificationChannel* CommunicationCentral::GetNotificationChannel()
{
	return &m_notifChannel;
}

void CommunicationCentral::ControllerConnected()
{
	m_discoveryChannel.SetResponseState( DiscoveryChannel::StateBusy );
}

void CommunicationCentral::ControllerDisconnected()
{
	if( m_pNotifyHandler != NULL )
	{
		m_pNotifyHandler->ControllerDisconnected();
	}
	
	ResetChannels();

	m_discoveryChannel.SetResponseState( DiscoveryChannel::StateAvailable );
}

bool CommunicationCentral::PrepareChannels( const char* in_pszControllerName )
{
	m_discoveryChannel.SetControllerName( in_pszControllerName );
	
	return m_commandChannel.StartListening()
			&& m_notifChannel.StartListening();
}

bool CommunicationCentral::ResetChannels()
{
	m_notifChannel.Term();
	m_commandChannel.Term();

	m_discoveryChannel.SetControllerName( NULL );

	return true;
}

void CommunicationCentral::WaitForChannelsReady()
{
	m_bWaitingForChannelsReady = true;
}
