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

class IChannelsHolder;
struct ControlMessage;

class ControllerChannel : public IncomingChannel
{
public:
	ControllerChannel( IChannelsHolder* in_pHolder, AkMemPoolId in_pool );
	virtual ~ControllerChannel();

	virtual bool Init();

	void SendChannelsReady();

protected:
	virtual bool ProcessCommand( Serializer& in_rSerializer );
	virtual void ControllerConnected( const GameSocketAddr& in_rControllerAddr );
	virtual void ControllerDisconnected();

private:

	IChannelsHolder* m_pHolder;

	DECLARE_BASECLASS( IncomingChannel );
};
