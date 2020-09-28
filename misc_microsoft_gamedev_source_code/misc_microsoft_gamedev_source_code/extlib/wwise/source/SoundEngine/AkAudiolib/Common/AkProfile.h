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

/***************************************************************************************************
**
** AkProfile.h
**
***************************************************************************************************/
#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "AkCommon.h"
#include "AkMonitorData.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkLock.h>
#include "AudioLibDefs.h"     // Pool IDs definition.

class AkPerf
{
public:
	static void Init();
	static void Term();

	static void TickAudio();

	static AkUInt32 GetPerfInterval() { return m_ulPerfInterval; }

	static void IncrementPrepareCount(){ ++m_ulPreparedEvents; }
	static void DecrementPrepareCount(){ --m_ulPreparedEvents; }

	static void IncrementBankMemory( AkUInt32 in_Size ) { m_ulBankMemory += in_Size; }
	static void DecrementBankMemory( AkUInt32 in_Size ) { m_ulBankMemory -= in_Size; }
	static void IncrementPreparedMemory( AkUInt32 in_Size ) { m_ulPreparedEventMemory += in_Size; }
	static void DecrementPreparedMemory( AkUInt32 in_Size ) { m_ulPreparedEventMemory -= in_Size; }


private:
	static void PostPluginTimers( AkInt64 in_iNow );
	static void PostMemoryStats( AkInt64 in_iNow );
    static void PostStreamingStats( AkInt64 in_iNow );
    static void PostPipelineStats( AkInt64 in_iNow );
	static void PostEnvironmentStats();
	static void PostObsOccStats();
	static void PostListenerStats();
	static void PostControllerStats();
	static void PostOutputStats();
	static void PostGameObjPositions();
	static void PostWatchGameSyncValues();
	static void PostInteractiveMusicInfo();
	static void PostFeedbackStats(AkReal32 in_fInterval);
	static void PostFeedbackDevicesStats();
	static AkInt64 m_iLastUpdateAudio;			// Last update (QueryPerformanceCounter value)
	static AkInt64 m_iLastUpdateCursorPosition;	// Last update (QueryPerformanceCounter value)

	static AkInt64 m_iLastUpdatePlugins;
	static AkInt64 m_iLastUpdateMemory;
    static AkInt64 m_iLastUpdateStreaming;
	static AkInt64 m_iLastUpdatePipeline;

	static AkUInt32 m_ulPerfInterval;	// Interval in milliseconds for periodic monitoring
	static AkUInt32 m_ulCursorPositionInterval;

	static AkUInt32 m_ulPreparedEvents;
	static AkUInt32 m_ulBankMemory;
	static AkUInt32 m_ulPreparedEventMemory;
};

extern AkMemPoolId	g_DefaultPoolId;

#if ( !defined(AK_OPTIMIZED) && !defined(XBOX) )

#define AK_PERF_TICK_AUDIO()					AkPerf::TickAudio()
#define AK_PERF_INTERVAL()						AkPerf::GetPerfInterval()
#define AK_PERF_INIT()							AkPerf::Init()
#define AK_PERF_TERM()							AkPerf::Term()
#define AK_PERF_INCREMENT_PREPARE_EVENT_COUNT()		AkPerf::IncrementPrepareCount()
#define AK_PERF_DECREMENT_PREPARE_EVENT_COUNT()		AkPerf::DecrementPrepareCount()
#define AK_PERF_INCREMENT_BANK_MEMORY( _size_ )		AkPerf::IncrementBankMemory( _size_ ) 
#define AK_PERF_DECREMENT_BANK_MEMORY( _size_ )		AkPerf::DecrementBankMemory( _size_ )
#define AK_PERF_INCREMENT_PREPARED_MEMORY( _size_ )	AkPerf::IncrementPreparedMemory( _size_ )
#define AK_PERF_DECREMENT_PREPARED_MEMORY( _size_ )	AkPerf::DecrementPreparedMemory( _size_ )

#else

#define AK_PERF_TICK_AUDIO()
#define AK_PERF_INTERVAL()						INFINITE
#define AK_PERF_INIT()
#define AK_PERF_TERM()
#define AK_PERF_INCREMENT_PREPARE_EVENT_COUNT()
#define AK_PERF_DECREMENT_PREPARE_EVENT_COUNT()
#define AK_PERF_INCREMENT_BANK_MEMORY(_size_)	
#define AK_PERF_DECREMENT_BANK_MEMORY(_size_)	
#define AK_PERF_INCREMENT_PREPARED_MEMORY(_size_)
#define AK_PERF_DECREMENT_PREPARED_MEMORY(_size_)

#endif

#endif
