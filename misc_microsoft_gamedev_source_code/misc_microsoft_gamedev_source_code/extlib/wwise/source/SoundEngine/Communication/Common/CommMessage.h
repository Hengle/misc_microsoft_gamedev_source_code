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

#include "AkPrivateTypes.h"

class Serializer;
/*
struct CommMessage
{
	CommMessage();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	AkUInt32 m_uiMsgLength;
};
*/

namespace CommMessage
{
	typedef AkUInt32 MsgLength;
	static AkUInt8 s_uiMsgLengthTypeSize = sizeof( MsgLength );
}

struct ControlMessage
{
public:
	enum Command
	{
		CmdPrepareChannels = 0,
		CmdWaitForChannelsReady,
		CmdResetChannels,
		CmdProbing
	};

	ControlMessage();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	Command m_eCommand;
	const char* m_pszControllerName;
};

struct CommandMessage
{
	enum Mode
	{
		ModeAsync = 0,
		ModeSync
	};

	CommandMessage();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	Mode m_eMode;
	AkUInt32 m_uiDataLength;
	AkUInt8* m_pData;
};

struct NotificationMessage
{
	NotificationMessage();

	bool Serialize( Serializer& in_rSerializer ) const;
	bool Deserialize( Serializer& in_rSerializer );

	AkUInt32 m_uiDataLength;
	AkUInt8* m_pData;
};
