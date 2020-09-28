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

#include "ControllerChannel.h"
#include "IChannelsHolder.h"
#include "CommMessage.h"
#include "Serializer.h"
#include "IPConnectorConstants.h"

ControllerChannel::ControllerChannel( IChannelsHolder* in_pHolder, AkMemPoolId in_pool )
	: IncomingChannel( IPConnectorConstants::kControlChannelPort, in_pool )
	, m_pHolder( in_pHolder )
{
}

ControllerChannel::~ControllerChannel()
{
}

bool ControllerChannel::Init()
{
	return StartListening() && __base::Init();
}

void ControllerChannel::SendChannelsReady()
{
	AkUInt8 retVal = true;
	Send( &retVal, 1 );
}

bool ControllerChannel::ProcessCommand( Serializer& in_rSerializer )
{
	ControlMessage msg;

	in_rSerializer.Get( msg );
	
	bool bRet = false;
	bool bMustReply = false;

	switch( msg.m_eCommand )
	{
	case ControlMessage::CmdPrepareChannels:
		bRet = m_pHolder->PrepareChannels( msg.m_pszControllerName );
		bMustReply = true;
		break;

	case ControlMessage::CmdWaitForChannelsReady:
		m_pHolder->WaitForChannelsReady();
		break;

	case ControlMessage::CmdResetChannels:
		bRet = m_pHolder->ResetChannels();
		bMustReply = true;
		break;

	case ControlMessage::CmdProbing:
		// Do nothing with this.
		break;
	}

	if( bMustReply )
	{
		AkUInt8 retVal = bRet;
		Send( &retVal, 1 );
	}

	return bRet;
}

void ControllerChannel::ControllerConnected( const GameSocketAddr& in_rControllerAddr )
{
	m_pHolder->ControllerConnected();
}

void ControllerChannel::ControllerDisconnected()
{
	m_pHolder->ControllerDisconnected();
}
