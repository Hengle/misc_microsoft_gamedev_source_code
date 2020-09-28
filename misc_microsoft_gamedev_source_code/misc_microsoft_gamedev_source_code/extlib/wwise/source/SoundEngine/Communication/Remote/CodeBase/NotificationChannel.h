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

#include "INotificationChannel.h"
#include "GameSocket.h"

class NotificationChannel : public AK::Comm::INotificationChannel
{
public:
	NotificationChannel();
	virtual ~NotificationChannel();

	// INotificationChannel members
	virtual void SendNotification( const AkUInt8* in_pNotificationData, int in_dataSize ) const;

	bool Init();
	void Term();

	bool StartListening();
	void StopListening();

	void Process();

	bool IsReady() const;

private:
	GameSocket m_serverSocket;
	GameSocket m_connSocket;
};
