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


#include "StdAfx.h"

#include "ConsoleDiscoveryMessage.h"
#include "Serializer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DiscoveryMessage::DiscoveryMessage( Type in_eType )
	: m_type( in_eType )
{
}

bool DiscoveryMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( m_uiMsgLength )
		&& in_rSerializer.Put( (AkInt32)m_type );
}

bool DiscoveryMessage::Deserialize( Serializer& in_rSerializer )
{
	return in_rSerializer.Get( m_uiMsgLength )
		&& in_rSerializer.Get( (AkInt32&)m_type );
}

DiscoveryRequest::DiscoveryRequest()
	: DiscoveryMessage( TypeDiscoveryRequest )
{
}

bool DiscoveryRequest::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer );
}

bool DiscoveryRequest::Deserialize( Serializer& in_rSerializer )
{
	return __base::Deserialize( in_rSerializer );
}

DiscoveryResponse::DiscoveryResponse()
	: DiscoveryMessage( TypeDiscoveryResponse )
{
}

bool DiscoveryResponse::Serialize( Serializer& in_rSerializer ) const
{
	return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_uiProtocolVersion )
		&& in_rSerializer.Put( (AkInt32)m_eConsoleType )
		&& in_rSerializer.Put( m_pszConsoleName )
		&& in_rSerializer.Put( (AkInt32)m_eConsoleState )
		&& in_rSerializer.Put( m_pszControllerName );
}

bool DiscoveryResponse::Deserialize( Serializer& in_rSerializer )
{
	AkInt32 iRead = 0;

	return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_uiProtocolVersion )
		&& in_rSerializer.Get( (AkInt32&)m_eConsoleType )
		&& in_rSerializer.Get( (char*&)m_pszConsoleName, iRead )
		&& in_rSerializer.Get( (AkInt32&)m_eConsoleState )
		&& in_rSerializer.Get( (char*&)m_pszControllerName, iRead );
}
