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


/*	
	----------------------------------------------------------------------------------------
	PC (Windows) AND Xbox360 implementation.
	Location: \Wwise\Communication\Remote\PC
	Header location: \Wwise\Communication\Remote\CodeBase

	-- IMPORTANT NOTE -- : Also used for the Xbox360 implementation, even though the implementation file is located in the PC folder!!
	-----------------------------------------------------------------------------------------
*/

#include "stdafx.h"

#include "GameSocket.h"
#include "GameSocketAddr.h"


GameSocket::GameSocket()
	: m_socket( INVALID_SOCKET )
{
}

GameSocket::GameSocket( const GameSocket& in_rGameSocket )
	: m_socket( in_rGameSocket.m_socket )
{
}

GameSocket::GameSocket( SOCKET in_socket )
	: m_socket( in_socket )
{
}

GameSocket::~GameSocket()
{
}

GameSocket& GameSocket::operator=( const GameSocket& in_rGameSocket )
{
	if( this != &in_rGameSocket )
	{
		m_socket = in_rGameSocket.m_socket;
	}

	return *this;
}

SOCKET GameSocket::Create( AkInt32 in_type, AkInt32 in_protocol )
{
	m_socket = ::socket( AF_INET, in_type, in_protocol );

	return m_socket;
}

AkInt32 GameSocket::ReuseAddress()
{
	BOOL bReuseAddr = TRUE;
	return ::setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof bReuseAddr );
}

AkInt32 GameSocket::SetSockOpt( AkInt32 in_level, AkInt32 in_optionName, char* in_optionVal, AkInt32 optionLength ) const
{
	return ::setsockopt( m_socket, in_level, in_optionName, in_optionVal, optionLength );
}

AkInt32 GameSocket::Connect( const GameSocketAddr& in_rAddr ) const
{
	return ::connect( m_socket, (sockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );
}

AkInt32 GameSocket::Bind( const GameSocketAddr& in_rAddr ) const
{
	return ::bind( m_socket, (sockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );
}

AkInt32 GameSocket::Listen( AkInt32 in_backlog ) const
{
	return ::listen( m_socket, in_backlog );
}

GameSocket GameSocket::Accept( GameSocketAddr& out_rAddr ) const
{
	int addrSize = sizeof( out_rAddr.GetInternalType() );
	return ::accept( m_socket, (sockaddr*)&out_rAddr.GetInternalType(), (int*)&addrSize );
}

AkInt32 GameSocket::Send( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const
{
	// Ensure that the whole buffer is sent.
	const char* pBuf = (const char*)in_pBuf;

	AkInt32 toSend = in_length;

	while( toSend > 0 )
	{
		AkInt32 sendVal = ::send( m_socket, pBuf, toSend, in_flags );

		// If there's an error or the socket has been closed by peer.
		if( sendVal == SOCKET_ERROR || sendVal == 0 )
			return sendVal;

		pBuf += sendVal;
		toSend -= sendVal;
	}

	return in_length - toSend;
}

AkInt32 GameSocket::Recv( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags ) const
{
	// Ensure that the whole buffer is sent.
	char* pBuf = (char*)in_pBuf;

	AkInt32 toRecv = in_length;

	while( toRecv > 0 )
	{
		 AkInt32 recvVal = ::recv( m_socket, pBuf, toRecv, 0 );

		 // If there's an error or the socket has been closed by peer.
		 if( recvVal == SOCKET_ERROR || recvVal == 0 )
			 return recvVal;

		 pBuf += recvVal;
		 toRecv -= recvVal;
	}

	return in_length - toRecv;
}

AkInt32 GameSocket::SendTo( const void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, const GameSocketAddr& in_rAddr ) const
{
	return ::sendto( m_socket, (const char*)in_pBuf, in_length, in_flags, (sockaddr*)&in_rAddr.GetInternalType(), sizeof( in_rAddr.GetInternalType() ) );
}

AkInt32 GameSocket::RecvFrom( void* in_pBuf, AkInt32 in_length, AkInt32 in_flags, GameSocketAddr& out_rAddr ) const
{
	int addrSize = sizeof( out_rAddr.GetInternalType() );	
	return ::recvfrom( m_socket, (char*)in_pBuf, in_length, in_flags, (sockaddr*)&out_rAddr.GetInternalType(), (int*)&addrSize );
}

AkInt32 GameSocket::Poll( PollType in_ePollType, AkUInt32 in_timeout ) const
{
	fd_set fds = { 0 };
	FD_SET( m_socket, &fds );
	timeval tv = { 0 };
	tv.tv_usec = in_timeout * 1000;

	return ::select( 0,
						in_ePollType == PollRead ? &fds : NULL, 
						in_ePollType == PollWrite ? &fds : NULL, 
						NULL, 
						&tv );
}

AkInt32 GameSocket::Shutdown( AkInt32 in_how ) const
{
	return ::shutdown( m_socket, in_how );
}

AkInt32 GameSocket::Close()
{
	AkInt32 result = ::closesocket( m_socket );
	m_socket = INVALID_SOCKET;

	return result;
}

SOCKET GameSocket::GetSocket() const
{
	return m_socket;
}

GameSocket::operator SOCKET() const
{
	return m_socket;
}
/*
AkInt32 GameSocket::Select( fd_set* in_readfds, fd_set* in_writefds, fd_set* exceptfds, const timeval* in_timeout )
{
	return ::select( 0, in_readfds, in_writefds, exceptfds, in_timeout );
}
*/