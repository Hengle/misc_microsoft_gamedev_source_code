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

#include "CommMessage.h"
#include "Serializer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
CommMessage::CommMessage()
	: m_uiMsgLength( 0 )
{
}

bool CommMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( m_uiMsgLength );
}

bool CommMessage::Deserialize( Serializer& in_rSerializer )
{
	return in_rSerializer.Get( m_uiMsgLength );
}
*/
ControlMessage::ControlMessage()
{
}

bool ControlMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( (AkInt32)m_eCommand )
		&& in_rSerializer.Put( m_pszControllerName );
}

bool ControlMessage::Deserialize( Serializer& in_rSerializer )
{
	AkInt32 dummy = 0;

	return in_rSerializer.Get( (AkInt32&)m_eCommand )
		&& in_rSerializer.Get( (char*&)m_pszControllerName, dummy );
}

CommandMessage::CommandMessage()
	: m_uiDataLength( 0 )
	, m_pData( NULL )
{
}

bool CommandMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( (AkInt32)m_eMode )
		&& in_rSerializer.Put( m_pData, m_uiDataLength );
}

bool CommandMessage::Deserialize( Serializer& in_rSerializer )
{
	return in_rSerializer.Get( (AkInt32&)m_eMode )
		&& in_rSerializer.Get( (void*&)m_pData, (AkInt32&)m_uiDataLength );
}

NotificationMessage::NotificationMessage()
	: m_uiDataLength( 0 )
	, m_pData( NULL )
{
}

bool NotificationMessage::Serialize( Serializer& in_rSerializer ) const
{
	return in_rSerializer.Put( m_pData, m_uiDataLength );
}

bool NotificationMessage::Deserialize( Serializer& in_rSerializer )
{
	return in_rSerializer.Get( (void*&)m_pData, (AkInt32&)m_uiDataLength );
}
