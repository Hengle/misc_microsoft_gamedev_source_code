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

#include "CommandChannel.h"
#include <AK/Comm/ICommandChannelHandler.h>
#include "CommMessage.h"
#include "Serializer.h"
#include "IPConnectorConstants.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CommandChannel::CommandChannel( AkMemPoolId in_pool )
	: IncomingChannel( IPConnectorConstants::kCommandChannelPort, in_pool )
	, m_pCmdChannelHandler( NULL )
{
	// AKASSERT( m_pCmdChannelHandler );
}

CommandChannel::~CommandChannel()
{
}

void CommandChannel::SetCommandChannelHandler( AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler )
{
	m_pCmdChannelHandler = in_pCmdChannelHandler;
}

bool CommandChannel::ProcessCommand( Serializer& in_rSerializer )
{
	CommandMessage msg;

	in_rSerializer.Get( msg );

	WriteBytesMem returnData;
	
	m_pCmdChannelHandler->HandleExecute( msg.m_pData, &returnData );
	
	if( returnData.Bytes() != NULL && returnData.Count() != 0 )
	{
		Send( returnData.Bytes(), returnData.Count() );
	}

	return true;
}

void CommandChannel::ControllerConnected( const GameSocketAddr& in_rControllerAddr )
{
}

void CommandChannel::ControllerDisconnected()
{
}
