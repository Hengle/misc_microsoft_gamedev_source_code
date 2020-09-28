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
// AkAudiolibTimer.h
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKAUDIOLIBTIMER_H_
#define _AKAUDIOLIBTIMER_H_

#include "AkKeyList.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

extern AkMemPoolId g_DefaultPoolId;

// NOTE: g_fFreqRatio was moved to namespace AK (for publication purposes).
// It is available in all builds.
namespace AK
{
    extern AkReal32 g_fFreqRatio;
}

#ifndef AK_OPTIMIZED

namespace AkAudiolibTimer
{
	class AkTimerItem
	{
	public:
		AkTimerItem()
		{
			memset( this, 0, sizeof( AkTimerItem ) );
		}

		void Start()
		{
			AKPLATFORM::PerformanceCounter( &LastTime );
		}

		void Stop()
		{
			AkInt64 i64NewTime;
			AKPLATFORM::PerformanceCounter( &i64NewTime );
			ActualUsage += (i64NewTime - LastTime);
		}

		void Stamp()
		{
			ComputedUsage = ActualUsage;
			ActualUsage = 0;
		}

		AkReal32 Millisecs()
		{
			AKASSERT( AK::g_fFreqRatio );
            return ComputedUsage / AK::g_fFreqRatio;
		}

	private:
		AkInt64 ComputedUsage;
		AkInt64 ActualUsage;
		AkInt64 LastTime;
	};

	struct AkPlugInTimerItem
	{
		AkPlugInTimerItem():uNumInstances(0){}
		AkTimerItem timer;
		AkUInt32 uNumInstances;
	};

	typedef CAkKeyList<AkUInt32, AkPlugInTimerItem, AkAllocAndFree> AkTimerMap;

	extern AkTimerItem timerAudio;
	extern AkTimerMap  g_PluginTimers;
	extern AkTimerItem timerFeedback;

	inline void InitTimers()
	{
        AKPLATFORM::UpdatePerformanceFrequency();

		g_PluginTimers.Init( 0, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId ); // 0 initial items required for thread safety
	}

	inline void TermTimers()
	{
		g_PluginTimers.Term();
	}

	inline void StartPluginTimer( AkUInt32 in_uiPluginID )
	{
		// Luckily this doesn't seem to pose any big problems while another thread is iterating g_PluginTimers
		AkPlugInTimerItem * pTimer = g_PluginTimers.Set( in_uiPluginID ); 
		AKASSERT( pTimer );
		if ( !pTimer )
			return; // ran out of space ? nothing we can do.

		pTimer->timer.Start();
	}

	inline void StopPluginTimer( AkUInt32 in_uiPluginID )
	{
		AkPlugInTimerItem * pTimer = g_PluginTimers.Exists( in_uiPluginID );
		if ( !pTimer )
			return;

		pTimer->timer.Stop();
	}

	inline void IncrementPlugInCount( AkUInt32 in_uiPluginID )
	{
		AkPlugInTimerItem * pTimer = g_PluginTimers.Set( in_uiPluginID );
		if ( !pTimer )
			return;

		pTimer->uNumInstances += 1;
	}

	inline void DecrementPlugInCount( AkUInt32 in_uiPluginID )
	{
		AkPlugInTimerItem * pTimer = g_PluginTimers.Exists( in_uiPluginID );
		if ( !pTimer )
			return;

		pTimer->uNumInstances -= 1;
	}
}

#define AK_INIT_TIMERS()			AkAudiolibTimer::InitTimers()	// call it once at init time
#define AK_TERM_TIMERS()			AkAudiolibTimer::TermTimers()   // call it once at term time

#define AK_START_TIMER_AUDIO()		AkAudiolibTimer::timerAudio.Start()
#define AK_STOP_TIMER_AUDIO()		AkAudiolibTimer::timerAudio.Stop()

#define AK_START_PLUGIN_TIMER( in_uiPluginID ) AkAudiolibTimer::StartPluginTimer( in_uiPluginID )
#define AK_STOP_PLUGIN_TIMER( in_uiPluginID )  AkAudiolibTimer::StopPluginTimer( in_uiPluginID )

#define AK_INCREMENT_PLUGIN_COUNT( in_uiPluginID ) AkAudiolibTimer::IncrementPlugInCount( in_uiPluginID )
#define AK_DECREMENT_PLUGIN_COUNT( in_uiPluginID )  AkAudiolibTimer::DecrementPlugInCount( in_uiPluginID )

#define AK_START_TIMER_FEEDBACK()	if (AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback) AkAudiolibTimer::timerFeedback.Start();
#define AK_STOP_TIMER_FEEDBACK()	if (AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback) AkAudiolibTimer::timerFeedback.Stop();

#else

// In release, do not create profiling timers, but compute global frequency ratio.
#define AK_INIT_TIMERS()            AKPLATFORM::UpdatePerformanceFrequency()
#define AK_TERM_TIMERS()

#define AK_START_TIMER_AUDIO()
#define AK_STOP_TIMER_AUDIO()

#define AK_START_PLUGIN_TIMER( in_uiPluginID ) 
#define AK_STOP_PLUGIN_TIMER( in_uiPluginID )  

#define AK_INCREMENT_PLUGIN_COUNT( in_uiPluginID )
#define AK_DECREMENT_PLUGIN_COUNT( in_uiPluginID )

#define AK_START_TIMER_FEEDBACK()
#define AK_STOP_TIMER_FEEDBACK()

#endif





#endif //_AKAUDIOLIBTIMER_H_
