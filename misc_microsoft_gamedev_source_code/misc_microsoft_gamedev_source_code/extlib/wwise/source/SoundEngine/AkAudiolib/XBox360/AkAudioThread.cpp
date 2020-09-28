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
// AkAudioThread.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkAudioThread.h"
#include "AkAudioMgr.h"
#include "AkRandom.h"
#include "AkLEngine.h"
#include "AkBus.h"

using namespace AKPLATFORM;

extern AkPlatformInitSettings			g_PDSettings;

AkThread		CAkAudioThread::m_hEventMgrThread;

CAkAudioThread::CAkAudioThread()
	:m_bStopThread(false)
{
	for ( int i=0; i<NumEventMgrEvents; i++ )
		m_eventMgrEvents[i] = NULL;
}

CAkAudioThread::~CAkAudioThread()
{
}

void CAkAudioThread::WakeupEventsConsumer()
{
	::SetEvent( m_eventMgrEvents[ThreadProcessEvent] );
}

//-----------------------------------------------------------------------------
// Name: EventMgrThreadFunc
// Desc: Audio loop
//-----------------------------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE(CAkAudioThread::EventMgrThreadFunc)
{
	AKRANDOM::AkRandomInit();

	// get our info from the parameter
    CAkAudioThread* pAudioThread = reinterpret_cast<CAkAudioThread*>(AK_THREAD_ROUTINE_PARAMETER);

	CAkLEngine::StartVoice();
	
	AKASSERT(g_pAudioMgr);

	DWORD dwWaitRes = WAIT_TIMEOUT;

	do
	{
		switch( dwWaitRes )
		{
		case X360ListenerEvent:
			pAudioThread->ProcessXBox360Notif();
			break;
		case X360VoiceEvent:
		case ThreadProcessEvent:
		case WAIT_TIMEOUT:			// Default Time Out
			g_pAudioMgr->Perform();
			break;
		default:
			AKASSERT( !"Unexpected event received on main thread" );
		}

		dwWaitRes = ::WaitForMultipleObjects( NumEventMgrEvents, pAudioThread->m_eventMgrEvents, FALSE, INFINITE ); 
		
    }
	while ( !pAudioThread->m_bStopThread );
    

    AkExitThread( AK_RETURN_THREAD_OK );
}

//-----------------------------------------------------------------------------
// Name: ProcessXBox360Notif
// Desc: Process XBox360 notifications
//
// Parameters:
//	None.
//
//-----------------------------------------------------------------------------
void CAkAudioThread::ProcessXBox360Notif()
{
	DWORD dwMsgFilter = 0;
	ULONG_PTR param = 0;
	if( XNotifyGetNext( m_eventMgrEvents[X360ListenerEvent], XN_XMP_PLAYBACKCONTROLLERCHANGED, &dwMsgFilter, &param ) )
	{
		bool bIsGameInControl = false;
		switch( dwMsgFilter )
		{   
			case XN_XMP_PLAYBACKCONTROLLERCHANGED: 
				bIsGameInControl = ( (BOOL)param != 0 );
				break;
			default:
				AKASSERT(!"Received unexpected notification from X360");
				break;
		}
		if( bIsGameInControl )
		{
			CAkBus::XMP_UnmuteBackgroundMusic();
		}
		else
		{
			CAkBus::XMP_MuteBackgroundMusic();
		}
	}
}


AKRESULT CAkAudioThread::Start()
{
	m_eventMgrEvents[X360ListenerEvent] = XNotifyCreateListener( XNOTIFY_XMP );
	m_eventMgrEvents[X360VoiceEvent] = CAkLEngine::GetVoiceEvent();	
	m_eventMgrEvents[ThreadProcessEvent] = ::CreateEvent( NULL, false, false, NULL );
	m_bStopThread	= false;

	for ( int i=0; i<NumEventMgrEvents; i++ )
	{
		if ( !m_eventMgrEvents[i] )
			return AK_Fail;
	}

	//Create the EventManagerThread
	AkCreateThread(	EventMgrThreadFunc,					// Start address
					this,								// Parameter
					&g_PDSettings.threadLEngine,		// Properties 
					&m_hEventMgrThread,					// AkHandle
					"AK::EventManager" );				// Debug name
	// is the thread ok ?
	if ( !AkIsValidThread(&m_hEventMgrThread) )
		return AK_Fail;
	return AK_Success;
}

void CAkAudioThread::Stop()
{
	m_bStopThread = true;
	if ( AkIsValidThread( &m_hEventMgrThread ) )
	{
		WakeupEventsConsumer();
		AkWaitForSingleThread( &m_hEventMgrThread );
		AkCloseThread(&m_hEventMgrThread);
	}

	if ( m_eventMgrEvents[X360ListenerEvent] )
		::CloseHandle( m_eventMgrEvents[X360ListenerEvent] );
	m_eventMgrEvents[X360ListenerEvent] = NULL;
	/** X360VoiceEvent does not belong to the upper engine
	if ( m_eventMgrEvents[X360VoiceEvent] )
		::CloseHandle( m_eventMgrEvents[X360VoiceEvent] );
		**/
	m_eventMgrEvents[X360VoiceEvent] = NULL;
	if ( m_eventMgrEvents[ThreadProcessEvent] )
		::CloseHandle( m_eventMgrEvents[ThreadProcessEvent] );
	m_eventMgrEvents[ThreadProcessEvent] = NULL;
}
