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
// AkAudioThread.h
//
// XBox360 specific implementation of thread audio renderer loop.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIO_THREAD_H_
#define _AUDIO_THREAD_H_

#include <AK/Tools/Common/AkPlatformFuncs.h>

class CAkAudioThread
{
public:
	CAkAudioThread();
	virtual ~CAkAudioThread();

	//Start/stop the Audio Thread
	AKRESULT Start();
	void Stop();

	void WakeupEventsConsumer();

private:

	void ProcessXBox360Notif();
	
	// Sound Thread function
    //
    // Return - CALLBACK - return value
    static AK_DECLARE_THREAD_ROUTINE(EventMgrThreadFunc);

	enum EventMgrEvents
	{
		X360ListenerEvent	= WAIT_OBJECT_0,
		X360VoiceEvent,
		ThreadProcessEvent,
		NumEventMgrEvents
	};
	HANDLE			m_eventMgrEvents[NumEventMgrEvents];
	bool			m_bStopThread;

	// EventMgr Thread Information
	static AkThread	m_hEventMgrThread;
};

#endif //_AUDIO_THREAD_H_
