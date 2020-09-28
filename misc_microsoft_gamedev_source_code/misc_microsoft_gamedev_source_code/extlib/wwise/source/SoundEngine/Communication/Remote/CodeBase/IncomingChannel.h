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

#include "GameSocket.h"
#include "CommMessage.h"

#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkLock.h>

class Serializer;

class IncomingChannel
{
public:
	IncomingChannel( unsigned short in_listenPort, AkMemPoolId in_pool );
	virtual ~IncomingChannel();

	virtual bool Init();
	virtual void Term();

	bool StartListening();
	void StopListening();

	void Process();

	bool IsReady() const;

protected:
	void Send( const AkUInt8* in_pData, int in_dataLength );

	virtual bool ProcessCommand( Serializer& in_rSerializer ) = 0;
	virtual void ControllerConnected( const GameSocketAddr& in_rControllerAddr ) = 0;
	virtual void ControllerDisconnected() = 0;

private:
	void Reset();
	void ProcessInboundConnection();
	void ProcessConnection();

	static AK_DECLARE_THREAD_ROUTINE( ProcessConnectionThreadFunc );

	unsigned short m_listenPort;

	AkMemPoolId m_pool;

	GameSocket m_serverSocket;
	GameSocket m_connSocket;

	AkThread m_hThread;
	bool m_bErrorProcessingConnection;
	CAkLock m_csReset;
};
