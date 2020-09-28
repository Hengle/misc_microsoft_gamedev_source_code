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

#include "CommunicationDefines.h"
#include "AkPrivateTypes.h"


class Serializer;

struct DiscoveryMessage
{
	enum Type
	{
		TypeDiscoveryRequest = 0,
		TypeDiscoveryResponse
	};

	DiscoveryMessage( Type in_eType );

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	AkUInt32 m_uiMsgLength;
	Type m_type;
};

struct DiscoveryRequest : public DiscoveryMessage
{
	DiscoveryRequest();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	DECLARE_BASECLASS( DiscoveryMessage );
};

struct DiscoveryResponse : public DiscoveryMessage
{
	enum ConsoleState
	{
		ConsoleStateBusy = 0,
		ConsoleStateAvailable
	};

	DiscoveryResponse();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	AkUInt32 m_uiProtocolVersion;
	CommunicationDefines::ConsoleType m_eConsoleType;
	const char* m_pszConsoleName;
	ConsoleState m_eConsoleState;
	const char* m_pszControllerName;

	DECLARE_BASECLASS( DiscoveryMessage );
};
