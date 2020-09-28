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

#include "IncomingChannel.h"

namespace AK
{
	namespace Comm
	{
		class ICommandChannelHandler;
	}
}

struct CommandMessage;

class CommandChannel : public IncomingChannel
{
public:
	CommandChannel( AkMemPoolId in_pool );
	virtual ~CommandChannel();

	void SetCommandChannelHandler( AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler );

protected:
	virtual bool ProcessCommand( Serializer& in_rSerializer );
	virtual void ControllerConnected( const GameSocketAddr& in_rControllerAddr );
	virtual void ControllerDisconnected();

private:
	AK::Comm::ICommandChannelHandler* m_pCmdChannelHandler;

	DECLARE_BASECLASS( IncomingChannel );
};
