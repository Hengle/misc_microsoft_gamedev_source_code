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

#include <AK/SoundEngine/Common/AkTypes.h>

#include "NotificationChannel.h"
#include "CommMessage.h"
#include "GameSocketAddr.h"
#include "IPConnectorConstants.h"
#include "Serializer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

NotificationChannel::NotificationChannel()
{
}

NotificationChannel::~NotificationChannel()
{
}

void NotificationChannel::SendNotification( const AkUInt8* in_pNotificationData, int in_dataSize ) const
{
	AKASSERT( m_connSocket.GetSocket() != INVALID_SOCKET );

	if( m_connSocket.GetSocket() != INVALID_SOCKET )
	{
		NotificationMessage msg;
		msg.m_pData = const_cast<AkUInt8*>( in_pNotificationData );
		msg.m_uiDataLength = in_dataSize;

		Serializer serializer( false );
		serializer.Put( msg );

		CommMessage::MsgLength msgLen = serializer.GetWrittenSize();
		serializer.Reset();
		serializer.Put( msgLen );
		serializer.Put( msg );

		m_connSocket.Send( (const char*)serializer.GetWrittenBytes(), serializer.GetWrittenSize(), 0 );
	}
}

bool NotificationChannel::Init()
{
	return true;
}

void NotificationChannel::Term()
{
	if( m_serverSocket.GetSocket() != INVALID_SOCKET )
	{
		m_serverSocket.Shutdown( SD_BOTH );
		m_serverSocket.Close();
	}

	if( m_connSocket.GetSocket() != INVALID_SOCKET )
	{
		m_connSocket.Shutdown( SD_BOTH );
		m_connSocket.Close();
	}
}

bool NotificationChannel::StartListening()
{
	m_serverSocket.Create( SOCK_STREAM, IPPROTO_TCP );
	m_serverSocket.ReuseAddress();

	GameSocketAddr localAddr( INADDR_ANY, IPConnectorConstants::kNotificationChannelPort );

	m_serverSocket.Bind( localAddr );
	
	return m_serverSocket.Listen( 1 ) != SOCKET_ERROR;
}

void NotificationChannel::StopListening()
{
	m_serverSocket.Shutdown( SD_BOTH );
	m_serverSocket.Close();
}

void NotificationChannel::Process()
{
	if( m_serverSocket.GetSocket() != INVALID_SOCKET )
	{
		int selVal = m_serverSocket.Poll( GameSocket::PollRead, 0 );

		if( selVal == SOCKET_ERROR )
		{
			// Socket closed by us
		}
		else if( selVal != 0 )
		{
			GameSocketAddr hostAddr;

			m_connSocket = m_serverSocket.Accept( hostAddr );

			if( m_connSocket.GetSocket() != INVALID_SOCKET )
			{
				StopListening();
//				ControllerConnected( hostAddr );
			}
		}
	}
}

bool NotificationChannel::IsReady() const
{
	return m_connSocket.GetSocket() != INVALID_SOCKET;
}
