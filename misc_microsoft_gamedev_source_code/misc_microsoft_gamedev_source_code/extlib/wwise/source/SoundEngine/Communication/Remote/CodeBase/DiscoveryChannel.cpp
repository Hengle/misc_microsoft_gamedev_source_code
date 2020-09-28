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

#include "DiscoveryChannel.h"
#include "GameSocketAddr.h"
#include "IPConnectorConstants.h"
#include "Network.h"
#include "Serializer.h"

#include <string.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

AkUInt32 DiscoveryChannel::s_uiProtocolVersion = -1;

DiscoveryChannel::DiscoveryChannel()
	: m_eState( StateAvailable )
{
	AKPLATFORM::AkMemSet( m_szComputerName, 0, sizeof m_szComputerName );

	AKPLATFORM::AkMemSet( m_szControllerName, 0, sizeof m_szControllerName );
}

DiscoveryChannel::~DiscoveryChannel()
{
}

bool DiscoveryChannel::Init()
{
	AkInt32 stringSize = sizeof m_szComputerName;
	Network::GetMachineName( m_szComputerName, &stringSize );

	m_socket.Create( SOCK_DGRAM, IPPROTO_UDP );

	BOOL bReuseAddr = TRUE;
	m_socket.SetSockOpt( SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof bReuseAddr );

	GameSocketAddr addr( INADDR_ANY, IPConnectorConstants::kDiscoveryBroadcastPort );

	return m_socket.Bind( addr ) != SOCKET_ERROR;
}

void DiscoveryChannel::Term()
{
	if( m_socket.GetSocket() != INVALID_SOCKET )
		m_socket.Close();
}

DiscoveryChannel::ResponseState DiscoveryChannel::GetResponseState() const
{
	return m_eState;
}

void DiscoveryChannel::SetResponseState( DiscoveryChannel::ResponseState in_eState )
{
	m_eState = in_eState;
}

void DiscoveryChannel::SetControllerName( const char* in_pszControllerName )
{
	if( in_pszControllerName != NULL )
		::strncpy( m_szControllerName, in_pszControllerName, sizeof m_szControllerName );
	else
		::memset( m_szControllerName, 0, sizeof m_szControllerName );
}

void DiscoveryChannel::Process()
{
	int selVal = m_socket.Poll( GameSocket::PollRead, 0 );

	if( selVal == SOCKET_ERROR )
	{
		// Socket closed by us
	}
	else if( selVal != 0 )
	{
		GameSocketAddr hostAddr;

		AkUInt8 recvBuf[512] = { 0 };
		
		// Receiving on UDP returns only the first available datagram.
		int recvVal = m_socket.RecvFrom( recvBuf, sizeof recvBuf, 0, hostAddr );

		if( recvVal == SOCKET_ERROR )
		{
			// Socket closed by us
		}
		else if( recvVal == 0 )
		{
			// Socket close by the host
		}
		else
		{
			DiscoveryRequest msg;
		
			Serializer serializer( !Network::SameEndianAsNetwork() );
			serializer.Deserializing( recvBuf );
			serializer.Get( msg );

			// Send back the response
			DiscoveryResponse response;
			response.m_uiProtocolVersion = s_uiProtocolVersion;
			response.m_eConsoleType = CommunicationDefines::g_eConsoleType;
			response.m_eConsoleState = ConvertStateToResponseType();
			response.m_pszConsoleName = m_szComputerName;
			response.m_pszControllerName = NULL;
			if( m_eState == StateBusy )
			 response.m_pszControllerName = m_szControllerName;

			hostAddr.SetPort( IPConnectorConstants::kDiscoveryResponsePort );

			serializer.Reset();
			{
				// Calculate the message length and set in our send message.
				serializer.Put( response );
				response.m_uiMsgLength = serializer.GetWrittenSize();
			}

			serializer.Reset();

			// Serialize the message for good.
			serializer.Put( response );
			
			m_socket.SendTo( (char*)serializer.GetWrittenBytes(), serializer.GetWrittenSize(), 0, hostAddr );
		}
	}
}

void DiscoveryChannel::SetProtocolVersion( AkUInt32 in_uiProtocolVersion )
{
	s_uiProtocolVersion = in_uiProtocolVersion;
}

DiscoveryResponse::ConsoleState DiscoveryChannel::ConvertStateToResponseType() const
{
	DiscoveryResponse::ConsoleState eState = DiscoveryResponse::ConsoleStateAvailable;

	switch( m_eState )
	{
	case StateAvailable:
		eState = DiscoveryResponse::ConsoleStateAvailable;
		break;

	case StateBusy:
		eState = DiscoveryResponse::ConsoleStateBusy;
		break;

	default:
		break;
		// Something went wrong
	}

	return eState;
}
