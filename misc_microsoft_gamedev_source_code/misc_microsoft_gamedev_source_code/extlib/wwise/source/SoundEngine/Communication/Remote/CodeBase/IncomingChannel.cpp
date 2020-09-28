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

#include "IncomingChannel.h"
#include "CommMessage.h"
#include "GameSocketAddr.h"
#include "Serializer.h"

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

using namespace AKPLATFORM;

IncomingChannel::IncomingChannel( unsigned short in_listenPort, AkMemPoolId in_pool )
	: m_listenPort( in_listenPort )
	, m_pool( in_pool )
	, m_bErrorProcessingConnection( false )
{
	AkClearThread( &m_hThread );
}

IncomingChannel::~IncomingChannel()
{
}

bool IncomingChannel::Init()
{
	return true;
}

void IncomingChannel::Term()
{
	Reset();
}

bool IncomingChannel::StartListening()
{
	m_serverSocket.Create( SOCK_STREAM, IPPROTO_TCP );

	BOOL bReuseAddr = TRUE;
	m_serverSocket.SetSockOpt( SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof bReuseAddr );

	GameSocketAddr localAddr( INADDR_ANY, m_listenPort );

	m_serverSocket.Bind( localAddr );
	
	return m_serverSocket.Listen( 5 ) != SOCKET_ERROR;
}

void IncomingChannel::StopListening()
{
	m_serverSocket.Shutdown( SD_BOTH );
	m_serverSocket.Close();
	m_serverSocket = INVALID_SOCKET;
}

void IncomingChannel::Process()
{
	ProcessInboundConnection();
	ProcessConnection();
}

bool IncomingChannel::IsReady() const
{
	return m_connSocket.GetSocket() != INVALID_SOCKET;
}

void IncomingChannel::Send( const AkUInt8* in_pData, int in_dataLength )
{
	// When serializing, the serializer puts the length of the data in front of the data
	// so it already fits our protocol { msg lenght | msg } pattern.
	Serializer serializer( false );
	serializer.Put( in_pData, in_dataLength );

	m_connSocket.Send( (const char*)serializer.GetWrittenBytes(), serializer.GetWrittenSize(), 0 );
}

void IncomingChannel::Reset()
{
	// It doesn't matter if the socket is INVALID_SOCKET or already unusable.
	m_serverSocket.Shutdown( SD_BOTH );
	m_connSocket.Shutdown( SD_BOTH );

	m_csReset.Lock();
	if( AkIsValidThread( &m_hThread ) )
	{
		AkWaitForSingleThread( &m_hThread );
		AkCloseThread( &m_hThread );
		AkClearThread( &m_hThread );
	}
	m_csReset.Unlock();

	// Must be done after we know the thread has quit, to avoid socket error 10038 (operation attempted on something that is not a socket).
	m_serverSocket.Close();
	m_connSocket.Close();

	m_bErrorProcessingConnection = false;
}

void IncomingChannel::ProcessInboundConnection()
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
				ControllerConnected( hostAddr );

				AKASSERT( !AkIsValidThread( &m_hThread ) );
				// Create thread w/ default settings.
				// PhM : Warning stack size may not be big enough.
				AkCreateThread( ProcessConnectionThreadFunc, this, NULL, &m_hThread, "AK::IncomingChannel" );
			}
		}
	}
}

void IncomingChannel::ProcessConnection()
{
	if( m_bErrorProcessingConnection )
	{
		Reset();
		ControllerDisconnected();
		StartListening();
	}
}

AK_DECLARE_THREAD_ROUTINE( IncomingChannel::ProcessConnectionThreadFunc )
{
	IncomingChannel& rThis = *reinterpret_cast<IncomingChannel*>( AK_THREAD_ROUTINE_PARAMETER );

	CommMessage::MsgLength msgLen = 0;
	AkUInt8* pRecvBuf = (AkUInt8*)AkAlloc( rThis.m_pool, 512 ); 
	AkUInt32 iBufSize = 512;

	Serializer serializer( false );

	while( true )
	{
		msgLen = 0;
		serializer.Reset();

		// Receive the message length
		int recvVal = rThis.m_connSocket.Recv( (char*)pRecvBuf, CommMessage::s_uiMsgLengthTypeSize, 0 );

		if( recvVal == 0 )
		{
			// We have the socket signaled, but we read nothing => socket closed by peer.
			rThis.m_bErrorProcessingConnection = true;

			break;
		}
		else if( recvVal == SOCKET_ERROR )
		{
//			const int iLastError = NetworkGetLastError();
//			TRACE1( "IncomingChannel::ProcessConnectionThreadFunc received with an error [%d]\n", iLastError );

			rThis.m_bErrorProcessingConnection = true;

			break;
		}
		else
		{
			serializer.Deserializing( pRecvBuf );
			serializer.Get( msgLen );

			if ( msgLen > iBufSize )
			{
				// grow buffer 
				iBufSize = msgLen;
				AkFree( rThis.m_pool, pRecvBuf );
				pRecvBuf = (AkUInt8*)AkAlloc( rThis.m_pool, iBufSize );
				
				if( pRecvBuf == NULL )
				{
					AKASSERT( pRecvBuf != NULL );
					rThis.m_bErrorProcessingConnection = true;
					break;
				}
			}

			// Receive the message itself.
			if( rThis.m_connSocket.Recv( (char*)pRecvBuf, msgLen, 0 ) > 0 )
			{
				serializer.Deserializing( pRecvBuf );
				rThis.ProcessCommand( serializer );
			}
		}
	}

	AkFree( rThis.m_pool, pRecvBuf );

	AkExitThread( AK_RETURN_THREAD_OK );
}
