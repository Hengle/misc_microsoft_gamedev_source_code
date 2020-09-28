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

//////////////////////////////////////////////////////////////////////
//
// IALMonitor.h
//
// gduford
//
//////////////////////////////////////////////////////////////////////
#ifndef _IALMONITOR_H_
#define _IALMONITOR_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkMonitorData.h"

namespace AkMonitorData
{
	struct Watch;
}

namespace AK
{
    class IALMonitorSink;

    class IALMonitor
    {
    public:
	    virtual void Register( IALMonitorSink* in_pMonitorSink, AkMonitorData::MonitorDataType in_whatToMonitor ) = 0;
	    virtual void Unregister( IALMonitorSink* in_pMonitorSink ) = 0;
		virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;
		virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;
    };
}

#endif	// _IALMONITOR_H_
