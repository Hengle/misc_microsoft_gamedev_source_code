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
#include "ConsoleDiscoveryMessage.h"

class DiscoveryChannel
{
public:
	DiscoveryChannel();
	~DiscoveryChannel();

	enum ResponseState
	{
		StateAvailable,
		StateBusy
	};

	bool Init();
	void Term();

	ResponseState GetResponseState() const;
	void SetResponseState( ResponseState in_eState );

	void SetControllerName( const char* in_pszControllerName );

	void Process();

	static void SetProtocolVersion( AkUInt32 in_uiProtocolVersion );

private:
	DiscoveryResponse::ConsoleState ConvertStateToResponseType() const;

	char m_szComputerName[16];
	char m_szControllerName[128];

	ResponseState m_eState;

	GameSocket m_socket;

	static AkUInt32 s_uiProtocolVersion;
};
