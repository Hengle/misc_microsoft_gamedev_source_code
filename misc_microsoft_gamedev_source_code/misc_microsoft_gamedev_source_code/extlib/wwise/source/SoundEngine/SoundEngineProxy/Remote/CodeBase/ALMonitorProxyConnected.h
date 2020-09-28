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

#include "ALMonitorProxyLocal.h"
#include "IALMonitorSink.h"

namespace AK
{
	namespace Comm
	{
		class INotificationChannel;
	}
}

class CommandDataSerializer;

class ALMonitorProxyConnected : public AK::IALMonitorSink
{
public:
	ALMonitorProxyConnected();
	virtual ~ALMonitorProxyConnected();

	// IALMonitorSink members
	virtual void MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem );
	
	// ALMonitorProxyConnected members
	void SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel );

	virtual void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rSerializer );

	ALMonitorProxyLocal& GetLocalProxy();

private:
	AK::Comm::INotificationChannel* m_pNotificationChannel;
	ALMonitorProxyLocal m_localProxy;
};
