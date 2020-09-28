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

#include "AkMonitorData.h"

class IALMonitorSubProxy
{
public:
	virtual void Monitor( AkMonitorData::MonitorDataType in_uWhatToMonitor ) = 0;
	virtual void StopMonitor() = 0;
	virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;
	virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;

	enum MethodIDs
	{
		MethodMonitor = 1,
		MethodStopMonitor,
		MethodSetWatches,
		MethodSetGameSyncWatches,

		LastMethodID
	};
};


